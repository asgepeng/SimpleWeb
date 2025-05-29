#include "login.h"
#include <iostream>

void LoginController::Index()
{
	view.layout = "_layout.html";
	view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
	view.sectionBody = R"(<form class="form-login" method="POST">
<label for="username">Username</label>
<div style="display:flex;flex-direction:column;">
<input type="text" id="username" name="username" value=""/>
<label for="txtpassword">Password</label>
<input type="password" id="txtpassword" name="txtpassword" value=""/>
<input type="submit" value="Login"/>
</div>
</form>)";
	view.sectionScripts = "<script src=\"/main.min.js\"><script>";
	std::string content = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
	context.response.Write(content);
	Send();
}

void LoginController::Index(Web::FormCollection& form)
{
	std::string username = form["username"];
	std::string password = form["txtpassword"];
	std::cout << "'" << username << "'" << std::endl;
	std::cout << "'" << password << "'" << std::endl;
	if (username == "admin" && password == "123")
	{
		context.response.Redirect("/home");
		Send();
	}
	else
	{
		context.response.Redirect("/");
		Send();
	}
}
