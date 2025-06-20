#include "db.h"
#include <iostream>

namespace Data
{
    std::string JsonEscape(const std::string& raw)
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
    std::string HtmlEncode(const std::string& data)
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

    DbClient::DbClient(const std::string& connectionString) : connStr(connectionString) {}
    DbClient::DbClient()
    {
        connStr = "Driver={ODBC Driver 17 for SQL Server};Server=.\\SQLEXPRESS;Database=PointOfSale;Trusted_Connection=Yes;";
    }
    DbClient::~DbClient()
    {
        Disconnect();
    }
    bool DbClient::Connect()
    {
        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS) return false;
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
        if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS) return false;

        auto ret = SQLDriverConnectA(
            hDbc, nullptr,
            (SQLCHAR*)connStr.c_str(), SQL_NTS,
            nullptr, 0, nullptr,
            SQL_DRIVER_NOPROMPT);

        return SQL_SUCCEEDED(ret);
    }
    void DbClient::Disconnect()
    {
        if (hDbc)
        {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            hDbc = nullptr;
        }
        if (hEnv)
        {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            hEnv = nullptr;
        }
    }
    std::string DbClient::ExecuteHtmlTable(const std::string& command)
    {
        std::string result;
        SQLHSTMT hStmt;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)command.c_str(), SQL_NTS)))
        {
            DbReader reader(hStmt);
            result = ExecuteHtmlTable(reader);
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return result;
    }
    std::string DbClient::ExecuteHtmlTable(Data::DbReader& reader)
    {
        std::ostringstream ss;
        ss << "<table class=\"table\">";

        auto columns = reader.GetColumnsInfo();
        size_t columnCount = columns.size();

        if (columnCount > 0)
        {
            ss << "<thead><tr>";
            for (const auto& column : columns)
            {
                ss << "<th>" << HtmlEncode(column.columnName) << "</th>";
            }
            ss << "</tr></thead><tbody>";
            while (reader.Read())
            {
                ss << "<tr>";
                for (short i = 0; i < columnCount; i++)
                {
                    const auto& column = columns[i];
                    short ordinal = i + 1;
                    SQLSMALLINT type = column.dataType;

                    std::string value;
                    switch (type)
                    {
                    case SQL_BIT:
                        value = reader.GetBoolean(ordinal) ? "Yes" : "No";
                        break;
                    default:
                        value = reader.GetString(ordinal);
                        break;
                    }

                    ss << "<td>" << HtmlEncode(value) << "</td>";
                }
                ss << "</tr>";
            }
            ss << "</tbody>";
        }
        ss << "</table>";
        return ss.str();
    }
    void DbClient::ExecuteJsonObject(const std::string& command, std::ostringstream& ss)
    {
        ss << "[";

        this->ExecuteReader(command, [&](DbReader reader)
            {
                auto columns = reader.GetColumnsInfo();
                int rowCount = 0;

                while (reader.Read())
                {
                    if (rowCount++ > 0) ss << ",\n";
                    ss << "{";

                    for (size_t i = 0; i < columns.size(); ++i)
                    {
                        if (i > 0) ss << ",";

                        const auto& column = columns[i];
                        const short columnOrdinal = static_cast<short>(i + 1);
                        const int type = column.dataType;

                        ss << "\"" << column.columnName << "\":";

                        std::string value;

                        switch (type)
                        {
                        case SQL_DOUBLE:
                            value = std::to_string(reader.GetDouble(columnOrdinal));
                            break;

                        case SQL_C_FLOAT:
                        case SQL_FLOAT:
                            value = std::to_string(reader.GetFloat(columnOrdinal));
                            break;

                        case SQL_DECIMAL:
                        case SQL_NUMERIC:
                            value = std::to_string(reader.GetDecimal(columnOrdinal));
                            break;

                        case SQL_BIT:
                            value = reader.GetBoolean(columnOrdinal) ? "true" : "false";
                            break;

                        case SQL_VARCHAR:
                        case SQL_CHAR:
                        case SQL_LONGVARCHAR:
                        {
                            auto str = reader.GetString(columnOrdinal);
                            value = "\"" + str + "\"";
                            break;
                        }
                        case SQL_TYPE_TIME:
                        case SQL_TYPE_TIMESTAMP:
                        {
                            auto str = JsonEscape(reader.GetString(columnOrdinal));
                            value = str.empty() ? "null" : "\"" + str + "\"";
                            break;
                        }

                        default:
                        {
                            auto str = reader.GetString(columnOrdinal);
                            value = str.empty() ? "null" : str;
                            break;
                        }
                        }

                        ss << value;
                    }

                    ss << "}";
                }
            });

        ss << "]";
    }
    void DbClient::ExecuteReader(const std::string& command, std::function<void(DbReader)> handler)
    {
        SQLHSTMT hStmt;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)command.c_str(), SQL_NTS)))
        {
            DbReader reader(hStmt);
            handler(reader);
        }
        else
        {
            // Ambil semua error diagnostic record
            SQLCHAR sqlState[6];
            SQLCHAR message[512];
            SQLINTEGER nativeError;
            SQLSMALLINT msgLen;
            SQLSMALLINT i = 1;

            std::cerr << "[ODBC ERROR] SQLExecDirectA failed:\n";
            while (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, i, sqlState, &nativeError, message, sizeof(message), &msgLen) == SQL_SUCCESS)
            {
                std::cerr << "  #" << i
                    << " SQLSTATE: " << sqlState
                    << ", Native Error: " << nativeError
                    << ", Message: " << message << "\n";
                ++i;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    }
    bool DbClient::ExecuteNonQuery(const std::string& command)
    {
        SQLHSTMT hStmt;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        bool success = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)command.c_str(), SQL_NTS));

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return success;
    }
    bool DbClient::ExecuteNonQuery(const std::string& command, std::vector<DbParameter>& parameters)
    {
        SQLHSTMT hStmt;
        if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt))) {
            std::cerr << "[ODBC ERROR] SQLAllocHandle failed.\n";
            return false;
        }

        if (!SQL_SUCCEEDED(SQLPrepareA(hStmt, (SQLCHAR*)command.c_str(), SQL_NTS))) {
            std::cerr << "[ODBC ERROR] SQLPrepare failed:\n";
            SQLCHAR sqlState[6], message[512];
            SQLINTEGER nativeError;
            SQLSMALLINT msgLen;
            SQLSMALLINT i = 1;
            while (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, i, sqlState, &nativeError, message, sizeof(message), &msgLen) == SQL_SUCCESS)
            {
                std::cerr << "  #" << i << " SQLSTATE: " << sqlState
                    << ", Native Error: " << nativeError
                    << ", Message: " << message << "\n";
                ++i;
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }

        std::vector<SQLLEN> indicators(parameters.size());

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            const DbParameter& p = parameters[i];

            bool bindSuccess = std::visit([&](auto&& val) -> bool {
                using T = std::decay_t<decltype(val)>;
                SQLPOINTER buffer = nullptr;
                SQLULEN columnSize = 0;
                SQLLEN& indicator = indicators[i];

                if constexpr (std::is_same_v<T, std::string>) {
                    buffer = (SQLPOINTER)val.c_str();

                    if (p.columnSize > 0) {
                        columnSize = p.columnSize;
                    }
                    else {
                        columnSize = static_cast<SQLULEN>(val.size() > 0 ? val.size() : 1);
                    }

                    indicator = static_cast<SQLLEN>(val.size());

                    return SQL_SUCCEEDED(SQLBindParameter(
                        hStmt,
                        static_cast<SQLUSMALLINT>(i + 1),
                        SQL_PARAM_INPUT,
                        p.cType,
                        p.sqlType,
                        columnSize,
                        0,
                        buffer,
                        0,
                        &indicator
                    ));
                }
                else {
                    buffer = (SQLPOINTER)&val;
                    indicator = sizeof(T);

                    return SQL_SUCCEEDED(SQLBindParameter(
                        hStmt,
                        static_cast<SQLUSMALLINT>(i + 1),
                        SQL_PARAM_INPUT,
                        p.cType,
                        p.sqlType,
                        0,
                        0,
                        buffer,
                        0,
                        &indicator
                    ));
                }
                }, p.value);

            if (!bindSuccess) {
                std::cerr << "[ODBC ERROR] SQLBindParameter failed at index " << i + 1 << "\n";

                SQLCHAR sqlState[6];
                SQLCHAR message[512];
                SQLINTEGER nativeError;
                SQLSMALLINT msgLen;
                SQLSMALLINT j = 1;
                while (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, j, sqlState, &nativeError, message, sizeof(message), &msgLen) == SQL_SUCCESS)
                {
                    std::cerr << "  [Bind Error #" << j << "] SQLSTATE: " << sqlState
                        << ", Native Error: " << nativeError
                        << ", Message: " << message << "\n";
                    ++j;
                }

                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                return false;
            }
        }

        bool execResult = SQL_SUCCEEDED(SQLExecute(hStmt));
        if (!execResult)
        {
            std::cerr << "[ODBC ERROR] SQLExecute failed:\n";
            SQLCHAR sqlState[6];
            SQLCHAR message[512];
            SQLINTEGER nativeError;
            SQLSMALLINT msgLen;
            SQLSMALLINT i = 1;

            while (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, i, sqlState, &nativeError, message, sizeof(message), &msgLen) == SQL_SUCCESS)
            {
                std::cerr << "  #" << i
                    << " SQLSTATE: " << sqlState
                    << ", Native Error: " << nativeError
                    << ", Message: " << message << "\n";
                ++i;
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return execResult;
    }
    bool DbClient::ExecuteLongText(const std::string& command, std::string& longText)
    {
        SQLHSTMT hStmt;
        SQLRETURN ret;

        // Allocate statement handle
        ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to allocate statement handle.\n";
            return false;
        }

        // Prepare the SQL statement (assume there's a ? placeholder for the text)
        ret = SQLPrepareA(hStmt, (SQLCHAR*)command.c_str(), SQL_NTS);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "SQLPrepare failed.\n";
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }

        // Bind the long text parameter
        SQLLEN cbTextLen = SQL_NTS; // SQL_NTS = null-terminated string
        ret = SQLBindParameter(
            hStmt,           // Statement handle
            1,               // Parameter number (1-based)
            SQL_PARAM_INPUT, // Input parameter
            SQL_C_CHAR,      // C type
            SQL_LONGVARCHAR, // SQL type for long text
            longText.length(), // Column size
            0,               // Decimal digits
            (SQLPOINTER)longText.c_str(), // Parameter value
            0,               // Buffer length (not needed for SQL_NTS)
            &cbTextLen       // Length/indicator
        );
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "SQLBindParameter failed.\n";
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }

        // Execute the statement
        ret = SQLExecute(hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "SQLExecute failed.\n";
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }

        // Clean up
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
    }
    int DbClient::ExecuteScalarInt(const std::string& query)
    {
        SQLHSTMT hStmt = nullptr;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return 0;
        }

        int value = 0;
        SQLLEN len = 0;
        if (SQLFetch(hStmt) == SQL_SUCCESS)
        {
            SQLGetData(hStmt, 1, SQL_C_SLONG, &value, sizeof(value), &len);
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return value;
    }
    std::string DbClient::ExecuteScalarString(const std::string& query)
    {
        SQLHSTMT hStmt = nullptr;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return "";
        }

        char buffer[1024] = {};
        SQLLEN len = 0;
        std::string result;
        if (SQLFetch(hStmt) == SQL_SUCCESS) {
            if (SQLGetData(hStmt, 1, SQL_C_CHAR, buffer, sizeof(buffer), &len) == SQL_SUCCESS && len != SQL_NULL_DATA) {
                result = buffer;
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return result;
    }
}
