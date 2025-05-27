#pragma once

namespace Web::Mvc
{
	class ActionResult
	{
	public:
		virtual void Execute(int socket) = 0;
	};

	class ViewResult
	{
	public:
		virtual void Execute(int socket);
	};
}

