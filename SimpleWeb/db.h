#pragma once
#include "dbreader.h"

#include <variant>

namespace Data
{
    struct DbParameter {
        std::variant<std::string, int, double> value;
        SQLSMALLINT cType;
        SQLSMALLINT sqlType;
        SQLULEN columnSize = 0;

        DbParameter(const std::string& v, SQLSMALLINT c, SQLSMALLINT s, SQLULEN cs)
            : value(v), cType(c), sqlType(s), columnSize(cs) {}

        DbParameter(const std::string& v, SQLSMALLINT c, SQLSMALLINT s)
            : value(v), cType(c), sqlType(s) {}

        DbParameter(int v, SQLSMALLINT c, SQLSMALLINT s)
            : value(v), cType(c), sqlType(s) {}

        DbParameter(int v, SQLSMALLINT c, SQLSMALLINT s, SQLULEN cs)
            : value(v), cType(c), sqlType(s), columnSize(cs) {}

        DbParameter(double v, SQLSMALLINT c, SQLSMALLINT s)
            : value(v), cType(c), sqlType(s) {}

        DbParameter(double v, SQLSMALLINT c, SQLSMALLINT s, SQLULEN cs)
            : value(v), cType(c), sqlType(s), columnSize(cs) {}
    };

    class DbClient {
    public:
        DbClient(const std::string& connectionString);
        DbClient();
        ~DbClient();

        bool Connect();
        void Disconnect();

        void ExecuteJsonObject(const std::string& command, std::ostringstream& ss);
        std::string ExecuteHtmlTable(const std::string& command);
        std::string ExecuteHtmlTable(Data::DbReader& reader);
        void ExecuteReader(const std::string& command, std::function<void(DbReader)> handler);
        bool ExecuteNonQuery(const std::string& command);
        bool ExecuteNonQuery(const std::string& command, std::vector<DbParameter>& parameter);
        bool ExecuteLongText(const std::string& command, std::string& longText);
        int ExecuteScalarInt(const std::string& query);
        std::string ExecuteScalarString(const std::string& query);
    private:
        std::string connStr;
        SQLHENV hEnv = nullptr;
        SQLHDBC hDbc = nullptr;
    };
}