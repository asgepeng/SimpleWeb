#include "mvc.h"

#include <fstream>
#include <sstream>
#include <algorithm>

std::string Web::Mvc::PageBuilder::RenderLayout(
    const std::string& layoutName,
    const std::string& sectionStyles,
    const std::string& bodyContent,
    const std::string& sectionScripts)
{
    std::string filePath = "E:\\PointOfSale\\SimpleWeb\\x64\\Release\\wwwroot\\shared\\" + layoutName;
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (!file)
    {
        return "";
    }
    std::string layoutContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto replace_all = [](std::string& str, const std::string& from, const std::string& to) 
        {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos)
            {
                str.replace(pos, from.length(), to);
                pos += to.length(); // Hindari replace berulang
            }
        };

    replace_all(layoutContent, "@sectionStyles", sectionStyles);
    replace_all(layoutContent, "@body", bodyContent);
    replace_all(layoutContent, "@sectionScripts", sectionScripts);

    return layoutContent;
}

HttpResponse& Web::Mvc::Controller::Redirect(const std::string& url)
{
    Response().Redirect(url);
    return Response();
}

HttpResponse& Web::Mvc::Controller::View(Layout& layout)
{
    Response().Write(R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
    <link href="/assets/main.min.css" rel="stylesheet"/>
</head>)");
    if (layout.styles != "") Response().Write(layout.styles);
    
    Response().Write(R"(<body>
<header>
<nav>
<ul>
<li>Home</li>
<li>Products</li>
<li>About</li>
<li>Contact Us</li>
</ul>
</nav>
</header>
<main class="main">)");
    Response().Write(layout.content);

    if (layout.scripts != "") Response().Write(layout.scripts);
    Response().Write(R"(<script src="/scripts/main.min.js"></script>
</main>
<footer>
</footer></body></html>)");
    return Response();
}
