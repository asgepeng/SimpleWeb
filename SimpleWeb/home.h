#pragma once
#include "routing.h"
#include "mvc.h"
#include "db.h"
#include "response.h"

#include <string>

class HomeController : Web::Mvc::Controller
{
public:
	HomeController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	static void MapRoute(Router* router);
	HttpResponse Index();
	HttpResponse Index(Web::FormCollection& form);
	HttpResponse Logout();
private:
	Data::DbClient db;
};

