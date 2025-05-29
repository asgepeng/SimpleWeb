#include "product.h"

void ProductController::Index()
{
    auto view = GetView();
    view.layout = "_layout.html";
    view.sectionStyles = "<link href=\"/assets/main.min.css\" rel=\"stylesheet\"/>";
    view.sectionScripts = "<script type=\"text/javascript\" src=\"/scripts/main.min.js\"></script>";

    if (db.Connect())
    {
        view.sectionBody = db.ExecuteHtmlTable("SELECT p.id, p.[name], p.[sku], c.[name] AS category, p.stock, p.unit, p.price, u.[name] AS [created by] FROM products AS p INNER JOIN categories AS c ON p.category = c.id INNER JOIN users AS u ON p.author = u.id WHERE p.deleted = 0");
        db.Disconnect();
    }
    std::string body = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
    GetResponse().Write(body);
    Send();
}

void ProductController::Edit(int id)
{
    auto view = GetView();
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
    std::string body = Web::Mvc::PageBuilder::RenderLayout(view.layout, view.sectionStyles, view.sectionBody, view.sectionScripts);
    GetResponse().Write(body);
    Send();
}