#include "wal/wal.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace customdb {

namespace {
char type_code(WalType type) {
    switch (type) {
    case WalType::Put: return 'P';
    case WalType::Delete: return 'D';
    case WalType::Begin: return 'B';
    case WalType::Commit: return 'C';
    case WalType::Rollback: return 'R';
    }
    return '?';
}

WalType parse_type(char code) {
    switch (code) {
    case 'P': return WalType::Put;
    case 'D': return WalType::Delete;
    case 'B': return WalType::Begin;
    case 'C': return WalType::Commit;
    case 'R': return WalType::Rollback;
    default: throw std::runtime_error("invalid WAL record type");
    }
}
}

Wal::Wal(const std::string& path) : path_(path) {
    std::ofstream create(path_, std::ios::app | std::ios::binary);
    if (!create) {
        throw std::runtime_error("unable to open WAL: " + path_);
    }
}

void Wal::append(const WalRecord& record) {
    std::ofstream output(path_, std::ios::app | std::ios::binary);
    if (!output) {
        throw std::runtime_error("unable to append WAL");
    }
    output << type_code(record.type) << '\t' << record.transaction_id << '\t'
           << record.key.size() << '\t' << record.key << '\t' << record.value.size() << '\t'
           << record.value << '\n';
    if (!output) {
        throw std::runtime_error("unable to write WAL record");
    }
}

void Wal::sync() {
    std::ofstream output(path_, std::ios::app | std::ios::binary);
    output.flush();
    if (!output) {
        throw std::runtime_error("unable to flush WAL");
    }
    output.close();
#ifdef _WIN32
    const int descriptor = _open(path_.c_str(), _O_RDWR | _O_BINARY);
    if (descriptor < 0 || _commit(descriptor) != 0) {
        if (descriptor >= 0) _close(descriptor);
        throw std::runtime_error("unable to durably sync WAL");
    }
    _close(descriptor);
#else
    const int descriptor = open(path_.c_str(), O_RDONLY);
    if (descriptor < 0 || fsync(descriptor) != 0) {
        if (descriptor >= 0) close(descriptor);
        throw std::runtime_error("unable to durably sync WAL");
    }
    close(descriptor);
#endif
}

void Wal::truncate() {
    std::ofstream output(path_, std::ios::trunc | std::ios::binary);
    if (!output) {
        throw std::runtime_error("unable to truncate WAL");
    }
}

std::vector<WalRecord> Wal::read_all() const {
    std::ifstream input(path_, std::ios::binary);
    std::vector<WalRecord> records;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream parser(line);
        char code = 0;
        TransactionId transaction_id = 0;
        std::size_t key_size = 0;
        std::size_t value_size = 0;
        std::string key;
        std::string value;
        char separator = 0;
        parser.get(code);
        parser.get(separator);
        parser >> transaction_id;
        parser.get(separator);
        parser >> key_size;
        parser.get(separator);
        key.resize(key_size);
        parser.read(key.data(), static_cast<std::streamsize>(key_size));
        parser.get(separator);
        parser >> value_size;
        parser.get(separator);
        value.resize(value_size);
        parser.read(value.data(), static_cast<std::streamsize>(value_size));
        if (!parser || key.size() != key_size || value.size() != value_size) {
            throw std::runtime_error("corrupt WAL record");
        }
        records.push_back({transaction_id, parse_type(code), key, value});
    }
    return records;
}

std::uint64_t Wal::next_transaction_id() const {
    std::uint64_t next = 1;
    for (const auto& record : read_all()) {
        if (record.transaction_id >= next) {
            next = record.transaction_id + 1;
        }
    }
    return next;
}

}  // namespace customdb
