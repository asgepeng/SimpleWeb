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


std::string Web::Mvc::View::ToString()
{
    std::string content = Web::Mvc::PageBuilder::RenderLayout(layout, sectionStyles, sectionBody, sectionScripts);
    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";

    std::string response;
    response.reserve(headers.size() + content.size());
    response.append(headers);
    response.append(content);

    return response;
}

void Web::Mvc::View::ExecuteResult()
{
}


std::string Web::Mvc::JsonResult::ToString()
{
    return std::string();
}
