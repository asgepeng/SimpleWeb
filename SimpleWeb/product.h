#pragma once
#include "mvc.h"
#include "web.h"
#include "db.h"

class ProductController : Web::Mvc::Controller
{
public:
	ProductController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	Web::Mvc::View Index();
private:
	Data::DbClient db;
};

