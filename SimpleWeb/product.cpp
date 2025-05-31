#include "product.h"

HttpResponse ProductController::Index()
{
    auto userId = Request().getCookie("user-login");
    if (userId == "")
    {
        return Redirect("/");
    }

    Web::Mvc::Layout layout;
    layout.styles = R"(<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Montserrat+Underline:ital,wght@0,100..900;1,100..900&family=Montserrat:ital,wght@0,100..900;1,100..900&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap" rel="stylesheet">)";

    if (db.Connect())
    {
        layout.content = db.ExecuteHtmlTable("SELECT p.id, p.[name], p.[sku], c.[name] AS category, p.stock, p.unit, p.price, u.[name] AS [created by] FROM products AS p INNER JOIN categories AS c ON p.category = c.id INNER JOIN users AS u ON p.author = u.id WHERE p.deleted = 0");
        db.Disconnect();
    }
    layout.scripts = R"(<script type="text/javascript">
document.addEventListener("DOMContentLoaded", function() {
    const rows = document.querySelectorAll('tr');
    rows.forEach(row => {
        row.addEventListener('click', function() {
            const firstCell = row.querySelector('td');
            if (firstCell) {
                const id = firstCell.textContent.trim();
                location.href = "/products/edit/" + encodeURIComponent(id);
            }
        });
    });
});
</script>)";
    return View(layout);
}

HttpResponse ProductController::Edit(int id)
{
    bool productFound = false;
    int category = 0;
    std::ostringstream oss;

    if (db.Connect())
    {
        std::string command = "SELECT [name], [description], [category] FROM products WHERE id = " + std::to_string(id);
        db.ExecuteReader(command, [&] (Data::DbReader reader) 
            {
                if (reader.Read())
                {
                    productFound = true;
                    oss << "<form method = \"POST\"><label for=\"textName\">Product Name</label>"
                        "<input id=\"textName\" name=\"productName\" value=\"" << reader.GetString(1) << "\"/>"
                        "<label for=\"textDesc\">Description</label>"
                        "<input id=\"textDesc\" name=\"desc\" value=\"" << reader.GetString(2) << "\"/>"
                        "<label for=\"category\">Kategori</label><select id=\"category\">";
                    category = reader.GetInt32(3);
                }
            });
        if (!productFound)
        {
            db.Disconnect();
            return Redirect("/products");
        }

        db.ExecuteReader("SELECT id, name FROM categories ORDER BY id ASC", [&](Data::DbReader reader)
            {
                while (reader.Read())
                {
                    int id = reader.GetInt32(1);
                    oss << "<option value=\"" << id << "\"";
                    if (id == category) oss << " selected";
                    oss << ">" << reader.GetString(2) << "</option>";
                }
            });
        db.Disconnect();
        oss << "</select><br><br><a class=\"button\" href=\"/products\">Back To List</a>";
    }

    if (!productFound) return Redirect("/products");

    Web::Mvc::Layout layout;
    layout.styles = R"(<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Montserrat+Underline:ital,wght@0,100..900;1,100..900&family=Montserrat:ital,wght@0,100..900;1,100..900&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap" rel="stylesheet">)";

    layout.content = oss.str();
    return View(layout);
}