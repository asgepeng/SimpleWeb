#pragma once
#include "request.h"

namespace Web
{
	class Middleware
	{
	public:
		static bool Authorize(std::string& token) { return true; }
	};
}

