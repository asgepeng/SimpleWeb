#pragma once
#include "routing.h"
#include "controllers.h"

namespace Web
{
	class ControllerRoutes : public RouteConfig
	{
	public:
		void RegisterEndPoints(Router* route) override {
            route->MapGet("/", [](Web::HttpContext& context) {
                LoginController login(context);
                return login.Index();
                });
            route->MapPost("/", [](Web::HttpContext& context) {
                LoginController login(context);
                Web::FormCollection form = context.request.getFormCollection();
                return login.Index(form);
                });
            route->MapGet("/home", [](Web::HttpContext& context) {
                HomeController home(context);
                return home.Index();
                });
            route->MapGet("/products", [](Web::HttpContext& context) {
                ProductController product(context);
                return product.Index();
                });
            route->MapGet("/products/edit/{id}", [](Web::HttpContext& context) {
                ProductController p(context);
                return p.Edit(std::stoi(context.routeData.at("id")));
                });
		}
	};
}