#include "db.h"
#include "dbinitializer.h"

#include <string>

void Data::DbInitializer::Initialize()
{
	bool exists = false;
	DbClient* db = new DbClient("Driver={ODBC Driver 17 for SQL Server};Server=.\\SQLEXPRESS;Database=master;Trusted_Connection=Yes;");
	if (db->Connect())
	{
		std::string commandText = R"(SELECT 1 AS col FROM sys.databases WHERE [name] = 'SimpleDb')";
		db->ExecuteReader(commandText, [&](DbReader reader)
			{
				exists = reader.Read();
			});
		if (!exists)
		{
			commandText = "CREATE DATABASE SimpleDb";
			db->ExecuteNonQuery(commandText);
		}
		db->Disconnect();
	}
	delete db;

	if (exists) return;

	std::string ddl = R"(CREATE TABLE users
(
	id INT NOT NULL IDENTITY(1, 1),
	[firstName] VARCHAR(50) NOT NULL CONSTRAINT df_user_firstName DEFAULT(''),
	[middleName] VARCHAR(50) NOT NULL CONSTRAINT df_user_middleName DEFAULT(''),
	[lastName] VARCHAR(50) NOT NULL CONSTRAINT df_user_lastName DEFAULT(''),
	username VARCHAR(50) NOT NULL UNIQUE,
	passwordHash VARCHAR(255) NOT NULL,
	email VARCHAR(100) NOT NULL,
	isActive BIT NOT NULL DEFAULT(1),
	createdAt DATETIME NOT NULL DEFAULT(GETDATE()),
	updatedAt DATETIME NULL,
	CONSTRAINT pk_users PRIMARY KEY CLUSTERED ([id] ASC)
);
CREATE TABLE roles
(
	id INT NOT NULL IDENTITY(1, 1),
	name VARCHAR(50) NOT NULL UNIQUE,
	description VARCHAR(255) NULL,
	CONSTRAINT pk_roles PRIMARY KEY ([id])
);
CREATE TABLE user_roles
(
	userId INT NOT NULL,
	roleId INT NOT NULL,
	PRIMARY KEY (userId, roleId),
	FOREIGN KEY (userId) REFERENCES users(id),
	FOREIGN KEY (roleId) REFERENCES roles(id)
);
CREATE TABLE pages
(
	id INT NOT NULL IDENTITY(1,1),
	title VARCHAR(200) NOT NULL,
	slug VARCHAR(200) NOT NULL UNIQUE,
	content TEXT NOT NULL,
	status VARCHAR(20) NOT NULL DEFAULT('draft'), -- draft/published/archived
	authorId INT NOT NULL,
	createdAt DATETIME NOT NULL DEFAULT(GETDATE()),
	updatedAt DATETIME NULL,
	FOREIGN KEY (authorId) REFERENCES users(id),
	CONSTRAINT pk_pages PRIMARY KEY ([id])
);
CREATE TABLE categories
(
	id INT NOT NULL IDENTITY(1,1),
	name VARCHAR(100) NOT NULL,
	slug VARCHAR(100) NOT NULL UNIQUE,
	description VARCHAR(255),
	CONSTRAINT pk_categories PRIMARY KEY ([id])
);
CREATE TABLE page_categories
(
	pageId INT NOT NULL,
	categoryId INT NOT NULL,
	PRIMARY KEY (pageId, categoryId),
	FOREIGN KEY (pageId) REFERENCES pages(id),
	FOREIGN KEY (categoryId) REFERENCES categories(id)
);
CREATE TABLE media
(
	id INT NOT NULL IDENTITY(1,1),
	fileName VARCHAR(255) NOT NULL,
	filePath VARCHAR(500) NOT NULL,
	mimeType VARCHAR(100),
	uploadedBy INT,
	uploadedAt DATETIME NOT NULL DEFAULT(GETDATE()),
	FOREIGN KEY (uploadedBy) REFERENCES users(id),
	CONSTRAINT pk_media PRIMARY KEY ([id])
);
CREATE TABLE logs
(
	id INT IDENTITY(1,1) PRIMARY KEY,
	userId INT,
	action VARCHAR(100),
	target VARCHAR(255),
	loggedAt DATETIME DEFAULT(GETDATE()),
	FOREIGN KEY (userId) REFERENCES users(id)
);)";
	db = new DbClient("Driver={ODBC Driver 17 for SQL Server};Server=.\\SQLEXPRESS;Database=SimpleDb;Trusted_Connection=Yes;");
	if (db->Connect())
	{
		db->ExecuteNonQuery(ddl);
		db->Disconnect();
	}
	delete db;
	
}
