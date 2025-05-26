#pragma once
#include "mvc.h"
#include "db.h"

#include <string>

class HomeController : Web::Mvc::Controller
{
public:
	HomeController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	Web::Mvc::View Index();
	std::string Index(Web::Mvc::FormCollection form);
private:
	Data::DbClient db;
};

