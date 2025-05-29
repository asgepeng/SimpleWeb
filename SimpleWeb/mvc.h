#pragma once
#include "web.h"
#include "action.h"

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

	class Controller
	{
	public:
		Controller(Web::HttpContext& ctx) : context(ctx), request(context.request), response(context.response) { }
	protected:
		HttpRequest& GetRequest() { return request; }
		HttpResponse& GetResponse() { return response; }

		void Send();
		ViewResult& GetView() { return view; }
		std::map<std::string, std::string>& GetViewData() { return viewData; }
	private:
		ViewResult view;
		Web::HttpContext& context;
		HttpRequest& request;
		HttpResponse& response;
		std::map<std::string, std::string> viewData;
	};
}

