#pragma once
#include "web.h"

#include <string>
#include <sstream>
#include <map>

namespace Web::Mvc
{
	class PageBuilder
	{
	public:
		static std::string RenderLayout(const std::string& layoutName, const std::string& sectionStyles, const std::string& bodyContent, const std::string& sectionScripts);
	};

	class FormCollection : std::map<std::string, std::string>
	{

	};

	class ActionResult
	{
	public:
		virtual std::string ToString() = 0;
	};

	class View : ActionResult
	{
	public:
		virtual std::string ToString();
		std::string layout = "";
		std::string sectionBody = "";
		std::string sectionStyles = "";
		std::string sectionScripts = "";
	};

	class JsonResult : ActionResult
	{
		virtual std::string ToString();
	};

	class Controller
	{
	public:
		Controller(Web::HttpContext& ctx) : context(ctx) { }
	protected:
		Web::HttpContext Context() { return context; }
		View view;
	private:
		Web::HttpContext context;
	};
}

