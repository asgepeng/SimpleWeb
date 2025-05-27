#include "dbreader.h"

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
        SQLRETURN ret = SQLGetData(SqlStatement, columnIndex, targetType, &value, sizeof(T), &len);
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
    float_t DbReader::GetFloat(short columnIndex)
    {
        return GetValue<float_t>(columnIndex, SQL_C_FLOAT, (float)0);
    }
    double_t DbReader::GetDouble(short columnIndex)
    {
        return GetValue<double_t>(columnIndex, SQL_C_DOUBLE, 0.0);
    }
    double_t DbReader::GetDecimal(short columnIndex)
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
        } while (ret == SQL_SUCCESS_WITH_INFO || bytesRead == SQL_NO_TOTAL);

        return buffer;
    }
}
