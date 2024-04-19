//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022 zSpace, Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_CORE_COMPATIBILITY_PLATFORM_DEFINES_H__
#define __ZSPACE_CORE_COMPATIBILITY_PLATFORM_DEFINES_H__

#if defined(_WIN32)
# define ZC_COMPAT_API_FUNC_CALL __stdcall
# define ZC_COMPAT_API_FUNC_PTR ZC_COMPAT_API_FUNC_CALL
#else
# define ZC_COMPAT_API_FUNC_CALL
# define ZC_COMPAT_API_FUNC_PTR ZC_COMPAT_API_FUNC_CALL
#endif // Platform

#endif // __ZSPACE_CORE_COMPATIBILITY_PLATFORM_DEFINES_H__
