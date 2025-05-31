#pragma once
#include "mvc.h"
#include "web.h"
#include "db.h"

class ProductController : Web::Mvc::Controller
{
public:
	ProductController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	HttpResponse Index();
	HttpResponse Edit(int id);
private:
	Data::DbClient db;
};

