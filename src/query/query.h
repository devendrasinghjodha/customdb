#pragma once

#include "api/database.h"
#include "transaction/transaction.h"
#include "query/plan.h"

#include <memory>
#include <string>
#include <vector>

namespace customdb {

enum class TokenType { Word, String, End };

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(std::string input);
    std::vector<Token> tokenize() const;

private:
    std::string input_;
};

class QueryExecutor {
public:
    explicit QueryExecutor(Database& database);
    std::string execute(const std::string& query);

private:
    Database& database_;
    std::unique_ptr<Transaction> transaction_;
    QueryPlanner planner_;
};

}  // namespace customdb
