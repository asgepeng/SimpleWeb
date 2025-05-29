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

void Web::Mvc::Controller::Send()
{
    std::string content = response.ToString();
    if (context.ssl)
    {
        SSL_write(context.ssl, content.c_str(), (int)content.size());
    }
    else {
        send(context.socket, content.c_str(), (int)content.size(), 0);
    }
}
