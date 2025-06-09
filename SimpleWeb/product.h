#pragma once
#include "routing.h"
#include "mvc.h"
#include "db.h"

class ProductController : Web::Mvc::Controller
{
public:
	ProductController(Web::HttpContext& context) : Web::Mvc::Controller(context) { }
	HttpResponse Index();
	HttpResponse Edit(int id);
	HttpResponse Edit(FormCollection& form);
	HttpResponse Add();
	HttpResponse Add(FormCollection form);
	HttpResponse Delete();
	HttpResponse Delete(int id);
	static void MapRoute(Router* router);
private:
	Data::DbClient db;
};

