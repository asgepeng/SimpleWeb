#include "home.h"
#include "response.h"

void HomeController::MapRoute(Router* router)
{
    router->MapGet("/home", [](Web::HttpContext& context) 
        {
            HomeController home(context);
            return home.Index();
        });
    router->MapGet("/home/logout", [](Web::HttpContext& context)
        {
            HomeController home(context);
            return home.Logout();
        });
}

HttpResponse HomeController::Index()
{
    auto view = GetView();
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";
    
    view.sectionBody = "<h1>Hello From C++ Web Server</h1><a href=\"/products\">Check it out</a>";
    std::string body = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
    Response().Write(body);
    return Response();
}

HttpResponse HomeController::Index(Web::FormCollection& form)
{
    return Response();
}

HttpResponse HomeController::Logout()
{
    Response().SetHeader("Set-Cookie", "user-login=; Max-Age=0; Path=/; HttpOnly; SameSite=Lax");
    return Redirect("/");
}
