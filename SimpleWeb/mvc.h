#pragma once
#include "action.h"
#include "httpcontext.h"

#include <string>
#include <sstream>
#include <map>

using namespace Web;
namespace Web::Mvc
{
	class PageBuilder
	{
	public:
		static std::string RenderLayout(const std::string& layoutName, const std::string& sectionStyles, const std::string& bodyContent, const std::string& sectionScripts);
	};

	struct Layout
	{
		std::string layoutName = "";
		std::string head = "";
		std::string styles = "";
		std::string scripts = "";
		std::string content = "";
	};

	class Controller
	{
	public:
		Controller(Web::HttpContext& ctx) : context(ctx) { }
	protected:
		HttpRequest Request() { return context.request; }
		HttpResponse& Response() { return context.response; }

		ViewResult& GetView() { return view; }
		std::map<std::string, std::string>& TempData() { return viewData; }

		HttpResponse& Redirect(const std::string& url);
		HttpResponse& View(Layout& layout);
	private:
		ViewResult view;
		Web::HttpContext& context;
		std::map<std::string, std::string> viewData;
	};
}

