#pragma once
#include "mvc.h"

using namespace Web::Mvc;

class PageController : public Web::Mvc::Controller
{
public:
	HttpResponse Index();
	HttpResponse Add();
	HttpResponse Add(FormCollection& form);
	HttpResponse Edit(std::string& id);
	HttpResponse Edit(FormCollection& form);
	HttpResponse Delete(std::string& id);
};
