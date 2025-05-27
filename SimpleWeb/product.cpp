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

Web::Mvc::View ProductController::Edit(int id)
{
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";
    
    std::string name = "--";
    std::string description = "--";

    if (db.Connect())
    {
        std::string command = "SELECT [name], [description] FROM products WHERE id = " + std::to_string(id);
        db.ExecuteReader(command, [&] (Data::DbReader reader) 
            {
                if (reader.Read())
                {
                    name = reader.GetString(1);
                    description = reader.GetString(2);
                }
            });
        db.Disconnect();
    }
    std::ostringstream oss;
    oss << "<form method=\"POST\"><label for=\"textId\">ID</label>"
        "<input id=\"textID\" name=\"id\" value=\"" << std::to_string(id) << "\"/>"
        "<label for=\"textName\">Product Name</label>"
        "<input id=\"textName\" name=\"productName\" value=\"" << name << "\"/>"
        "<label for=\"textDesc\">Description</label>"
        "<input id=\"textDesc\" name=\"desc\" value=\"" << description << "\"/>"
        "</form>";
    view.sectionBody = oss.str();
    return view;
}
