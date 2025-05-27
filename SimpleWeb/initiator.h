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
                HomeController home(context);
                return home.Index().ToString();
                });
            route->MapGet("/products", [](Web::HttpContext& context) {
                ProductController product(context);
                return product.Index().ToString();
                });
            route->MapGet("/products/edit/{id}", [](Web::HttpContext& context) {
                ProductController p(context);
                return p.Edit(std::stoi(context.RouteData.at("id"))).ToString();
                });
		}
	};
}