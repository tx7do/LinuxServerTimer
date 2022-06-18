#pragma once

#if !defined(LinuxServerTimer_API)
#	if defined (__GNUC__) && (__GNUC__ >= 4)
#		define LinuxServerTimer_API __attribute__ ((visibility ("default")))
#	else
#		define LinuxServerTimer_API
#	endif
#endif
