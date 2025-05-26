#include "home.h"

Web::Mvc::View HomeController::Index()
{
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";
    
    view.sectionBody = "<h1>Hello From C++ Web Server</h1><a href=\"/products\">Check it out</a>";
    return view;
}

std::string HomeController::Index(Web::Mvc::FormCollection form)
{
    return std::string();
}
