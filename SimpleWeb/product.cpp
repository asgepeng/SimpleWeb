#include "product.h"
#include "layoutmanager.h"
#include "sqlhelper.h"

#include <iostream>

HttpResponse ProductController::Index()
{
    auto userId = Request().GetCookie("user-login");
    if (userId == "")
    {
        return Redirect("/");
    }
    Response().Write(LayoutManager::layouts["main"]);
    if (db.Connect())
    {
        Response().Write(db.ExecuteHtmlTable("SELECT p.id, p.[name], p.[sku], c.[name] AS category, p.stock, p.unit, p.price, u.[name] AS [created by] FROM products AS p INNER JOIN categories AS c ON p.category = c.id INNER JOIN users AS u ON p.author = u.id WHERE p.deleted = 0"));
        db.Disconnect();
    }
    Response().Write(R"(</main>
<script type="text/javascript" src="/scripts/main.min.js"></script>
</body>
</html>)");
    return Response();
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
                    oss << "<form method = \"POST\" action=\"/products/edit\"><label for=\"textName\">Product Name</label>"
                        "<input type=\"hidden\" name=\"id\" value=\"" << id << "\"/>"
                        "<input id=\"textName\" name=\"productName\" value=\"" << reader.GetString(1) << "\"/>"
                        "<label for=\"editor\">Description</label>"
                        "<textarea id=\"editor\" name=\"desc\">" << reader.GetString(2) << "</textarea>"
                        "<label for=\"category\">Kategori</label><select id=\"category\" name=\"category\">";
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
        oss << R"(</select><br><br>
<button type="submit" class="button">Save</button>
<a class="button" href="/products">Back To List</a>)";
    }

    Web::Mvc::Layout layout;
    layout.styles = R"(<link rel="stylesheet" href="/scripts/ckeditor/ckeditor5/ckeditor5.css">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Montserrat+Underline:ital,wght@0,100..900;1,100..900&family=Montserrat:ital,wght@0,100..900;1,100..900&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap" rel="stylesheet">)";
    layout.scripts = R"(<script type="module">
    import { ClassicEditor, Essentials, Paragraph, Bold, Italic, Font } from "/scripts/ckeditor/ckeditor5/ckeditor5.js";
	ClassicEditor
		.create( document.querySelector( '#editor' ), {
            licenseKey: 'GPL',
			plugins: [ Essentials, Paragraph, Bold, Italic, Font ],
			toolbar: [
				'undo', 'redo', '|', 'bold', 'italic', '|',
				'fontSize', 'fontFamily', 'fontColor', 'fontBackgroundColor'
			]
		})
		.then( editor => {
			window.editor = editor;
		})
		.catch( error => {
			console.error( error );
		});
</script>)";
    layout.content = oss.str();
    return View(layout);
}

HttpResponse ProductController::Edit(FormCollection& form)
{
    int productId = std::stoi(form["id"]);
    std::string productName = form["productName"];
    std::string description = form["desc"];
    int category = std::stoi(form["category"]);

    std::string sql = R"(UPDATE products
SET [name] = ?, 
    [description] = ?, 
    [category] = ?
WHERE [id] = ?)";

    std::vector<Data::DbParameter> parameters = 
    {
        {productName, SQL_C_CHAR, SQL_VARCHAR, productName.size()},
        {description, SQL_C_CHAR, SQL_LONGVARCHAR, description.size()},
        {category, SQL_C_LONG, SQL_INTEGER, sizeof(int)},
        {productId, SQL_C_LONG, SQL_INTEGER, sizeof(int)}
    };

    if (db.Connect())
    {
        db.ExecuteNonQuery(sql, parameters);
        db.Disconnect();
    }

    return Redirect("/products");
}

HttpResponse ProductController::Add()
{
    return HttpResponse();
}

HttpResponse ProductController::Delete()
{
    return HttpResponse();
}

HttpResponse ProductController::Delete(int id)
{
    return HttpResponse();
}

void ProductController::MapRoute(Router* router)
{
    router->MapGet("/products", [](Web::HttpContext& context) {
        ProductController product(context);
        return product.Index();
        });
    router->MapGet("/products/edit/{id}", [](Web::HttpContext& context) {
        ProductController p(context);
        return p.Edit(std::stoi(context.routeData.at("id")));
        });
    router->MapPost("/products/edit/", [](Web::HttpContext& context)
        {
            ProductController p(context);
            Web::FormCollection form = context.request.getFormCollection();
            return p.Edit(form);
        });
}
