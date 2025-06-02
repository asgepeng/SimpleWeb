#include "login.h"
#include "sqlhelper.h"

void LoginController::MapRoute(Router* router)
{
	router->MapGet("/", [](Web::HttpContext& context) {
		LoginController login(context);
		return login.Index();
		});
	router->MapPost("/", [](Web::HttpContext& context) {
		LoginController login(context);
		Web::FormCollection form = context.request.getFormCollection();
		return login.Index(form);
		});
}

HttpResponse LoginController::Index()
{
	auto userID = Request().getCookie("user-login");
	if (userID != "")
	{
		return Redirect("/products");
	}

	Response().Write(R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
	<link rel="preconnect" href="https://fonts.googleapis.com">
	<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
	<link href="https://fonts.googleapis.com/css2?family=Montserrat+Underline:ital,wght@0,100..900;1,100..900&family=Montserrat:ital,wght@0,100..900;1,100..900&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap" rel="stylesheet">
    <link href="/assets/main.min.css" rel="stylesheet"/>
</head><body>)");
	auto it = TempData().find("invalid-login");
	if (it != TempData().end())
	{
		Response().Write("<div class=\"invalid-login\"><span>Invalid Login</span></div>");
	}
	Response().Write(R"(<form class="form-login" method="POST">
<img src="/images/logo.png"/ width="128" height="auto">
<label for="username">Username</label>
<div style="display:flex;flex-direction:column;">
<input type="text" id="username" name="username" value=""/>
<label for="txtpassword">Password</label>
<input type="password" id="txtpassword" name="txtpassword" value=""/></br>
<input type="submit" value="Login"/>
</div>
</form></body></html>)");
	return Response();
}

HttpResponse LoginController::Index(Web::FormCollection& form)
{
	std::string username = form["username"];
	std::string password = form["txtpassword"];

	std::string sql = "SELECT TOP(1)[id] FROM users WHERE[login] = '" + Helpers::SqlHelper::ConverText(username) + "' AND[password] = HASHBYTES('SHA2_256', '" + Helpers::SqlHelper::ConverText(password) + "')";

	int id = 0;
	if (db.Connect())
	{
		db.ExecuteReader(sql, [&](Data::DbReader reader)
			{
				if (reader.Read())
				{
					id = reader.GetInt32(1);
				}
			}
		);
		db.Disconnect();
	}

	if (id > 0)
	{
		Response().SetHeader("Set-Cookie", "user-login=" + std::to_string(id));
		return Redirect("/products");
	}
	else
	{
		TempData()["invalid-login"] = "1";
		return Index();
	}
}
