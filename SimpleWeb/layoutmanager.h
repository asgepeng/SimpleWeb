#pragma once
#include <string>
#include <unordered_map>

struct HTML
{

};
class LayoutManager
{
public:
	static void Load();
	static std::unordered_map<std::string, std::string> layouts;
};

