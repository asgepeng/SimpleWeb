#pragma once
#include "routing.h"
#include "mvc.h"
#include "db.h"

using namespace Web;

class LoginController : Web::Mvc::Controller
{
public:
	LoginController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	static void MapRoute(Router* router);
	HttpResponse Index();
	HttpResponse Index(Web::FormCollection& form);
private:
	Data::DbClient db;
};
