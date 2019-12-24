#ifndef SQLITEPP_DATABASE_H
#define SQLITEPP_DATABASE_H

#include <memory>
#include <string>
#include <system_error>
#include <cassert>

#include "sqlite3_inc.h"
#include "sqlitepp_stmt.h"

namespace sqlitepp
{

class database
{
public:
    database() noexcept = default;
    database(const char* db);
    database(const std::string& db);
    database(const char* db, int& result) noexcept;
    database(const std::string& db, int& result) noexcept;
    database(const database&) = delete;
    database(database&&) noexcept;
    database& operator=(const database&) = delete;
    database& operator=(database&&) noexcept;
    ~database() noexcept;

    int open(const char* db, int flags) noexcept;
    int open(const std::string& db, int flags) noexcept;
    int close() noexcept;

    template <typename... Args>
    statement prepare(const char* query, Args... args);
    statement prepare(const char* query);
    int execute(const char* query);

    int toggle_extended_result_codes() noexcept;
    bool is_using_extended_result_codes() const noexcept;

private:
    sqlite3* m_handle = nullptr;
    bool m_extended_result_codes = false;

}; // database

template <typename... Args>
statement database::prepare(const char* query, Args... args)
{
    statement stmt = prepare(query);
    stmt.bind(args...);

    return stmt;
}

} // sqlitepp

#endif // SQLITEPP_DATABASE_H
