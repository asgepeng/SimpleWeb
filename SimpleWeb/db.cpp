#include "db.h"
#include <iostream>

namespace Data
{
    DbReader::DbReader(SQLHSTMT& hStmt) 
    {
        SqlStatement = hStmt;
    }
    bool DbReader::Read()
    {
        return SQLFetch(SqlStatement) == SQL_SUCCESS;
    }
    std::vector<DbColumn> DbReader::GetColumnsInfo()
    {
        std::vector<DbColumn> columns;

        SQLSMALLINT numCols = 0;
        SQLRETURN ret = SQLNumResultCols(SqlStatement, &numCols);
        if (!SQL_SUCCEEDED(ret)) {
            return columns;
        }

        for (SQLUSMALLINT i = 1; i <= numCols; ++i)
        {
            SQLWCHAR colName[256];
            SQLSMALLINT nameLen;
            SQLSMALLINT dataType;
            SQLULEN colSize;
            SQLSMALLINT decimalDigits;
            SQLSMALLINT nullable;

            ret = SQLDescribeCol(SqlStatement, i, colName, sizeof(colName) / sizeof(SQLWCHAR), &nameLen,
                &dataType, &colSize, &decimalDigits, &nullable);

            if (SQL_SUCCEEDED(ret))
            {
                DbColumn col;
                col.columnName = std::string(reinterpret_cast<char*>(colName), nameLen * sizeof(SQLWCHAR));
                col.dataType = dataType;
                col.colSize = colSize;
                col.decimalDigit = decimalDigits;
                col.nullable = nullable;
                columns.push_back(col);
            }
        }
        return columns;
    }

    template<typename T>
    T DbReader::GetValue(short columnIndex, SQLSMALLINT targetType, T defaultValue)
    {
        T value = {};
        SQLLEN len = 0;
        SQLRETURN ret = SQLGetData(SqlStatement, columnIndex + 1, targetType, &value, sizeof(T), &len);
        return SQL_SUCCEEDED(ret) && len != SQL_NULL_DATA ? value : defaultValue;
    }
    bool DbReader::GetBoolean(short columnIndex)
    {
        return GetValue<char>(columnIndex, SQL_C_BIT, false) != 0;
    }
    int16_t DbReader::GetInt16(short columnIndex)
    {
        return GetValue<int16_t>(columnIndex, SQL_C_SSHORT, (int16_t)0);
    }
    int32_t DbReader::GetInt32(short columnIndex)
    {
        return GetValue<int32_t>(columnIndex, SQL_C_SLONG, (int32_t)0);
    }
    int64_t DbReader::GetInt64(short columnIndex)
    {
        return GetValue<int64_t>(columnIndex, SQL_C_SBIGINT, (int64_t)0);
    }
    float DbReader::GetFloat(short columnIndex)
    {
        return GetValue<float>(columnIndex, SQL_C_FLOAT, (float)0);
    }
    double DbReader::GetDouble(short columnIndex)
    {
        return GetValue<double>(columnIndex, SQL_C_DOUBLE, 0.0);
    }
    double DbReader::GetDecimal(short columnIndex)
    {
        auto decValue = GetString(columnIndex);
        return std::stod(decValue);
    }
    std::string DbReader::GetString(short columnIndex)
    {
        char buffer[1024] = {};
        SQLLEN len = 0;

        SQLRETURN ret = SQLGetData(SqlStatement, columnIndex, SQL_C_CHAR, buffer, sizeof(buffer), &len);
        return SQL_SUCCEEDED(ret) && len != SQL_NULL_DATA ? std::string(buffer) : "";
    }
    std::vector<uint8_t> DbReader::GetStream(const SQLSMALLINT ordinal)
    {
        std::vector<uint8_t> buffer;
        const size_t chunkSize = 4096;
        uint8_t tempBuffer[chunkSize];
        SQLLEN bytesRead = 0;

        SQLRETURN ret;
        do
        {
            ret = SQLGetData(SqlStatement, ordinal, SQL_C_BINARY, tempBuffer, chunkSize, &bytesRead);

            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
            {
                if (bytesRead > 0)
                {
                    size_t validBytes = (bytesRead > chunkSize || bytesRead == SQL_NO_TOTAL)
                        ? chunkSize
                        : static_cast<size_t>(bytesRead);
                    buffer.insert(buffer.end(), tempBuffer, tempBuffer + validBytes);
                }
            }
            else if (ret == SQL_NO_DATA)
            {
                break;
            }
            else
            {
                throw std::runtime_error("SQLGetData failed to read binary stream.");
            }
        } while (ret == SQL_SUCCESS_WITH_INFO || bytesRead == SQL_NO_TOTAL);

        return buffer;
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
        std::ostringstream ss;
        ss << "<table class=\"table\">";
        ExecuteReader(command, [&](Data::DbReader reader)
            {
                auto columns = reader.GetColumnsInfo();
                size_t columnCount = columns.size();

                if (columnCount > 0) 
                {
                    ss << "<thead><tr>";
                    for (const auto& column : columns) 
                    {
                        ss << "<th>" << Convert::HtmlEncode(column.columnName) << "</th>";
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

                            ss << "<td>" << Convert::HtmlEncode(value) << "</td>";
                        }
                        ss << "</tr>";
                    }
                    ss << "</tbody>";
                }
            });
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
                            auto str = Convert::JsonEscape(reader.GetString(columnOrdinal));
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
