#pragma once
#include <string>
#include "web.h"

namespace Web::Mvc
{
	class ActionResult
	{
	public:
		virtual std::string ToString() = 0;
	};

	class View : ActionResult
	{
	public:
		std::string layout;
	};

	class Controller
	{
	public:
		Controller(Web::HttpContext& ctx) : context(ctx) { }
		void Redirect(const std::string url);
		void RedirectToAction(const std::string actionName);
	protected:
		Web::HttpContext Context() { return context; }
	private:
		Web::HttpContext context;
	};
}

