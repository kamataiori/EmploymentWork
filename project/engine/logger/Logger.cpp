#include "Logger.h"

#include <dxgi1_6.h>

namespace Logger
{
	void Logger::Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}

}

