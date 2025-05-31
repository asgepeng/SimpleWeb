#pragma once
#include "mvc.h"
#include "web.h"
#include "db.h"

using namespace Web;

class LoginController : Web::Mvc::Controller
{
public:
	LoginController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	HttpResponse Index();
	HttpResponse Index(Web::FormCollection& form);
private:
	Data::DbClient db;
};
