#include "query/query.h"

#include "transaction/transaction.h"

#include <cctype>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace customdb {

Lexer::Lexer(std::string input) : input_(std::move(input)) {}

std::vector<Token> Lexer::tokenize() const {
    std::vector<Token> tokens;
    for (std::size_t index = 0; index < input_.size();) {
        if (std::isspace(static_cast<unsigned char>(input_[index])) || input_[index] == ';') {
            ++index;
            continue;
        }
        if (input_[index] == '(' || input_[index] == ')' || input_[index] == ',' || input_[index] == '=') {
            tokens.push_back({TokenType::Word, std::string(1, input_[index++])});
            continue;
        }
        if (input_[index] == '"') {
            ++index;
            std::string value;
            while (index < input_.size() && input_[index] != '"') {
                value.push_back(input_[index++]);
            }
            if (index == input_.size()) {
                throw std::invalid_argument("unterminated quoted string");
            }
            ++index;
            tokens.push_back({TokenType::String, value});
            continue;
        }
        const auto start = index;
        while (index < input_.size() && !std::isspace(static_cast<unsigned char>(input_[index])) && input_[index] != ';') {
            ++index;
        }
        tokens.push_back({TokenType::Word, input_.substr(start, index - start)});
    }
    tokens.push_back({TokenType::End, {}});
    return tokens;
}

QueryExecutor::QueryExecutor(Database& database) : database_(database) {}

std::string QueryExecutor::execute(const std::string& query) {
    const auto tokens = Lexer(query).tokenize();
    if (tokens.empty() || tokens[0].type == TokenType::End) {
        return {};
    }
    const auto command = tokens[0].text;
    auto upper = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });
        return value;
    };
    const auto normalized_command = upper(command);
    auto argument = [&tokens](std::size_t index) -> const std::string& {
        if (index >= tokens.size() || tokens[index].type == TokenType::End) {
            throw std::invalid_argument("missing query argument");
        }
        return tokens[index].text;
    };
    if (normalized_command == "CREATE" && upper(argument(1)) == "TABLE") {
        const auto table = argument(2);
        if (argument(3) != "(") throw std::invalid_argument("expected (");
        std::ostringstream schema;
        for (std::size_t index = 4; tokens[index].type != TokenType::End && tokens[index].text != ")"; ++index) {
            if (tokens[index].text != ",") {
                if (schema.tellp() > 0) schema << ' ';
                schema << tokens[index].text;
            }
        }
        database_.put("__schema__:" + table, schema.str());
        return "TABLE_CREATED";
    }
    if (normalized_command == "INSERT" && upper(argument(1)) == "INTO") {
        const auto table = argument(2);
        if (upper(argument(3)) != "VALUES" || argument(4) != "(") throw std::invalid_argument("expected VALUES (");
        std::ostringstream row;
        for (std::size_t index = 5; tokens[index].type != TokenType::End && tokens[index].text != ")"; ++index) {
            if (tokens[index].text != ",") {
                if (row.tellp() > 0) row << '\x1f';
                row << tokens[index].text;
            }
        }
        const auto counter_key = "__next__:" + table;
        const auto current = database_.get(counter_key);
        const auto row_id = current ? std::stoull(*current) : 0;
        database_.put("__row__:" + table + ":" + std::to_string(row_id), row.str());
        database_.put(counter_key, std::to_string(row_id + 1));
        return "INSERTED " + std::to_string(row_id);
    }
    if (normalized_command == "SELECT") {
        const auto table = upper(argument(1)) == "FROM" ? argument(2) : argument(3);
        if (upper(argument(1)) != "FROM" && (argument(1) != "*" || upper(argument(2)) != "FROM")) {
            throw std::invalid_argument("expected SELECT * FROM table");
        }
        std::optional<std::string> equality_value;
        if (tokens.size() > 7 && upper(tokens[4].text) == "WHERE" && upper(tokens[5].text) == "VALUE" && tokens[6].text == "=") {
            equality_value = tokens[7].text;
        }
        const auto plan = planner_.plan_select(table, equality_value);
        std::vector<std::pair<std::string, std::string>> rows;
        if (plan.access_path == AccessPath::SecondaryIndex) {
            for (const auto& key : database_.find_by_value(*plan.equality_value)) {
                if (key.rfind("__row__:" + table + ":", 0) == 0) {
                    const auto value = database_.get(key);
                    if (value) rows.emplace_back(key, *value);
                }
            }
        } else {
            rows = database_.scan_prefix("__row__:" + table + ":");
        }
        std::ostringstream output;
        for (std::size_t index = 0; index < rows.size(); ++index) {
            if (index != 0) output << '\n';
            output << rows[index].first.substr(rows[index].first.rfind(':') + 1) << " | " << rows[index].second;
        }
        return output.str();
    }
    if (normalized_command == "UPDATE") {
        const auto table = argument(1);
        if (upper(argument(2)) != "SET" || argument(4) != "=" || upper(argument(6)) != "WHERE" || upper(argument(7)) != "ID" || argument(8) != "=") {
            throw std::invalid_argument("expected UPDATE table SET value = value WHERE id = id");
        }
        const auto row_key = "__row__:" + table + ":" + argument(9);
        if (!database_.get(row_key)) return "NOT_FOUND";
        database_.put(row_key, argument(5));
        return "UPDATED";
    }
    if (normalized_command == "DELETE" && upper(argument(1)) == "FROM") {
        if (upper(argument(3)) != "WHERE" || argument(4) != "id" || argument(5) != "=") {
            throw std::invalid_argument("expected DELETE FROM table WHERE id = id");
        }
        const auto row_key = "__row__:" + argument(2) + ":" + argument(6);
        return database_.remove(row_key) ? "DELETED" : "NOT_FOUND";
    }
    if (command == "PUT" || command == "put") {
        const auto& key = argument(1);
        const auto& value = argument(2);
        if (transaction_) transaction_->put(key, value); else database_.put(key, value);
        return "OK";
    }
    if (command == "GET" || command == "get") {
        const auto value = transaction_ ? transaction_->get(argument(1)) : database_.get(argument(1));
        return value ? *value : "NOT_FOUND";
    }
    if (command == "DELETE" || command == "delete") {
        const auto& key = argument(1);
        if (transaction_) { transaction_->remove(key); return "OK"; }
        return database_.remove(key) ? "OK" : "NOT_FOUND";
    }
    if (command == "BEGIN" || command == "begin") {
        if (transaction_) throw std::logic_error("transaction already active");
        transaction_ = database_.begin();
        return "TXN " + std::to_string(transaction_->id()) + " started";
    }
    if (command == "COMMIT" || command == "commit") {
        if (!transaction_) throw std::logic_error("no active transaction");
        transaction_->commit();
        transaction_.reset();
        return "COMMITTED";
    }
    if (command == "ROLLBACK" || command == "rollback") {
        if (!transaction_) throw std::logic_error("no active transaction");
        transaction_->rollback();
        transaction_.reset();
        return "ROLLED_BACK";
    }
    if (command == "COMPACT" || command == "compact") {
        if (transaction_) throw std::logic_error("cannot compact during a transaction");
        database_.compact();
        return "COMPACTED";
    }
    if (command == "FIND" || command == "find") {
        const auto keys = database_.find_by_value(argument(1));
        std::ostringstream output;
        for (std::size_t index = 0; index < keys.size(); ++index) {
            if (index != 0) output << '\n';
            output << keys[index];
        }
        return output.str();
    }
    if (normalized_command == "METRICS") {
        const auto metrics = database_.metrics();
        return "keys=" + std::to_string(metrics.key_count) + " pages=" + std::to_string(metrics.page_count) +
               " wal_bytes=" + std::to_string(metrics.wal_bytes);
    }
    if (normalized_command == "BACKUP") {
        database_.backup_to(argument(1));
        return "BACKUP_CREATED";
    }
    throw std::invalid_argument("unknown query command");
}

}  // namespace customdb
