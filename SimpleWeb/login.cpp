#include "login.h"
#include "sqlhelper.h"

void LoginController::Index()
{
	auto view = GetView();
	view.layout = "_login_layout.html";
	view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
	auto it = GetViewData().find("invalid-login");
	if (it != GetViewData().end())
	{
		view.sectionBody = "<div class=\"invalid-login\"><span>Invalid Login</span></div>";
	}
	view.sectionBody += R"(<form class="form-login" method="POST">
<img src="/images/logo.png"/ width="128" height="auto">
<label for="username">Username</label>
<div style="display:flex;flex-direction:column;">
<input type="text" id="username" name="username" value=""/>
<label for="txtpassword">Password</label>
<input type="password" id="txtpassword" name="txtpassword" value=""/></br>
<input type="submit" value="Login"/>
</div>
</form>)";

	view.sectionScripts = "<script src=\"/main.min.js\"><script>";
	std::string content = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
	GetResponse().Write(content);
	Send();
}

void LoginController::Index(Web::FormCollection& form)
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
		GetResponse().Redirect("/home");
		Send();
	}
	else
	{
		GetViewData()["invalid-login"] = "1";
		Index();
	}
}
