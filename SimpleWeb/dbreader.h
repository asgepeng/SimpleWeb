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
        float_t GetFloat(short columnIndex);
        double_t GetDouble(short columnIndex);
        double_t GetDecimal(short columnIndex);
        std::string GetString(short columnIndex);
        std::vector<uint8_t> GetStream(const SQLSMALLINT ordinal);
    private:
        SQLHSTMT SqlStatement;
        template<typename T>
        T GetValue(short columnIndex, SQLSMALLINT targetType, T defaultValue = {});
    };
}