#include "product.h"

Web::Mvc::View ProductController::Index()
{
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";

    if (db.Connect())
    {
        view.sectionBody = db.ExecuteHtmlTable("SELECT * FROM products");
        db.Disconnect();
    }
    return view;
}
