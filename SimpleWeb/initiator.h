#pragma once
#include "routing.h"
#include "controllers.h"

namespace Web
{
	class ControllerRoutes : public RouteConfig
	{
	public:
		void RegisterEndPoints(Router* route) override 
		{
            LoginController::MapRoute(route);
            HomeController::MapRoute(route);
			ProductController::MapRoute(route);
		}
	};
}