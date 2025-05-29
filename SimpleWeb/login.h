#pragma once
#include "mvc.h"
#include "web.h"
#include "db.h"

class LoginController : Web::Mvc::Controller
{
public:
	LoginController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	void Index();
	void Index(Web::FormCollection& form);
private:
	Data::DbClient db;
};
