/// Syntacore SCR* framework
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief platform specific configurations

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#ifndef PLATFORM
#define PLATFORM unknown
#endif

#ifdef PLATFORM_HDR
#include PLATFORM_HDR
#endif

// platform info
#ifndef PLF_CPU_NAME
#define PLF_CPU_NAME "unknown"
#endif
#ifndef PLF_IMPL_STR
#define PLF_IMPL_STR "unknown"
#endif

#ifndef PLF_UART_BAUDRATE
#define PLF_UART_BAUDRATE 115200
#endif

#endif // PLATFORM_CONFIG_H
