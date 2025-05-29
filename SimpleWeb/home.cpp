#include "home.h"
#include "response.h"

void HomeController::Index()
{
    auto view = GetView();
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";
    
    view.sectionBody = "<h1>Hello From C++ Web Server</h1><a href=\"/products\">Check it out</a>";
    std::string body = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
    GetResponse().Write(body);
    Send();
}

void HomeController::Index(Web::FormCollection& form)
{
    
}
