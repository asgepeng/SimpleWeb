#include "sqlhelper.h"

std::string Helpers::SqlHelper::ConverText(std::string& value)
{
	std::string result;
	result.reserve(value.size() * 2);

	for (char ch : value)
	{
		if (ch == '\'')
		{
			result += "''";
		}
		else if (ch != '\0')
		{
			result += ch;
		}
	}
	return result;
}
