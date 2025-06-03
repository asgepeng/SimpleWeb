#include "db.h"
#include "dbinitializer.h"

#include <string>

void Data::DbInitializer::Initialize()
{
	DbClient db("Driver={ODBC Driver 17 for SQL Server};Server=.\\SQLEXPRESS;Database=master;Trusted_Connection=Yes;");
	if (db.Connect())
	{
		std::string commandText = R"(SELECT 1 AS col FROM sys.databases WHERE [name] = 'SimpleDb')";
		bool exists = false;
		db.ExecuteReader(commandText, [&](DbReader reader)
			{
				exists = reader.Read();
			});
		if (!exists)
		{
			commandText = "CREATE DATABASE SimpleDb";
			db.ExecuteNonQuery(commandText);
		}
		db.Disconnect();
	}
}
