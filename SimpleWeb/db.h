#pragma once
#include <string>
#include <functional>
#include <windows.h>
#include <sqlext.h>
#include <sstream>
#include <vector>
#include <iomanip>

namespace Data
{
    class Convert
    {
    public:
        std::string static JsonEscape(const std::string& raw)
        {
            std::ostringstream oss;
            for (char c : raw)
            {
                switch (c)
                {
                case '\"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b";  break;
                case '\f': oss << "\\f";  break;
                case '\n': oss << "\\n";  break;
                case '\r': oss << "\\r";  break;
                case '\t': oss << "\\t";  break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        oss << "\\u"
                            << std::hex << std::uppercase << std::setfill('0') << std::setw(4)
                            << (int)(unsigned char)c;
                    }
                    else
                    {
                        oss << c;
                    }
                    break;
                }
            }
            return oss.str();
        }
        static std::string HtmlEncode(const std::string& data)
        {
            std::ostringstream encoded;
            for (char c : data) {
                switch (c) {
                case '&': encoded << "&amp;"; break;
                case '<': encoded << "&lt;"; break;
                case '>': encoded << "&gt;"; break;
                case '"': encoded << "&quot;"; break;
                case '\'': encoded << "&#39;"; break;
                default: encoded << c; break;
                }
            }
            return encoded.str();
        }
    };

    struct NumberFormat : std::numpunct<char> {
    protected:
        char do_thousands_sep() const override { return '.'; }
        std::string do_grouping() const override { return "\3"; }
    };

    struct DbColumn
    {
        std::string columnName = "";
        SQLSMALLINT dataType = -1;
        SQLULEN colSize = 0;
        SQLSMALLINT decimalDigit = 0;
        SQLSMALLINT nullable = 0;
    };

    class DbReader
    {
    public:
        DbReader(SQLHSTMT& hStmt);
        bool Read();
        std::vector<DbColumn> GetColumnsInfo();
        
        bool GetBoolean(short columnIndex);
        int16_t GetInt16(short columnIndex);
        int32_t GetInt32(short columnIndex);
        int64_t GetInt64(short columnIndex);
        float GetFloat(short columnIndex);
        double GetDouble(short columnIndex);
        double GetDecimal(short columnIndex);
        std::string GetString(short columnIndex);
        std::vector<uint8_t> GetStream(const SQLSMALLINT ordinal);
    private:
        SQLHSTMT SqlStatement;
        template<typename T>
        T GetValue(short columnIndex, SQLSMALLINT targetType, T defaultValue = {});
    };

    class DbClient {
    public:
        DbClient(const std::string& connectionString);
        DbClient();
        ~DbClient();

        bool Connect();
        void Disconnect();

        void ExecuteJsonObject(const std::string& command, std::ostringstream& ss);
        void ExecuteHtmlTable(const std::string& command, std::ostringstream& ss);
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