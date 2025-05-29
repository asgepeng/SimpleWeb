#pragma once
#include "mvc.h"
#include "db.h"
#include "response.h"

#include <string>

class HomeController : Web::Mvc::Controller
{
public:
	HomeController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	void Index();
	void Index(Web::FormCollection& form);
private:
	Data::DbClient db;
};

