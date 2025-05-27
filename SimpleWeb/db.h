#pragma once
#include "dbreader.h"

namespace Data
{
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
        int ExecuteScalarInt(const std::string& query);
        std::string ExecuteScalarString(const std::string& query);
    private:
        std::string connStr;
        SQLHENV hEnv = nullptr;
        SQLHDBC hDbc = nullptr;
    };
}