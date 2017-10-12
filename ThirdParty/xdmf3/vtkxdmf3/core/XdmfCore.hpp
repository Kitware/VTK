/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCore.hpp                                                        */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef _XDMF_HPP
#ifndef _XDMFCORE_HPP
#define _XDMFCORE_HPP

#include "XdmfCoreConfig.hpp"

/* Keep all our Win32 Conversions here */
#ifdef _WIN32
# ifdef XDMFSTATIC
#   define XDMFCORE_EXPORT
#   define XDMFCORE_TEMPLATE
# else
  /* Used to export/import from the dlls */
#  ifdef XdmfCore_EXPORTS
#    define XDMFCORE_EXPORT __declspec(dllexport)
#    define XDMFCORE_TEMPLATE
#   else /* Xdmf_EXPORTS */
#    define XDMFCORE_EXPORT __declspec(dllimport)
#    define XDMFCORE_TEMPLATE extern
#   endif /* Xdmf_EXPORTS */
# endif

/* Used in XdmfSystemUtils */
#define PATH_MAX _MAX_PATH
#define realpath(x,y) _fullpath((char *) y,x, _MAX_PATH)

/* Compiler Warnings */
#ifndef XDMF_DEBUG
#pragma warning( disable : 4231 ) /* nonstandard extension used : 'extern' before template explicit instantiation */
#pragma warning( disable : 4251 ) /* needs to have dll-interface to be used by clients (Most of these guys are in private */
#pragma warning( disable : 4275 ) /* non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class */
#pragma warning( disable : 4373 ) /* virtual function overrides,  parameters only differed by const/volatile qualifiers */
#pragma warning( disable : 4101 ) /* 'exception' : unreferenced local variable */
#pragma warning( disable : 4355 ) /* 'this' : used in base member initializer list */
#pragma warning( disable : 4748 ) /* /GS can not protect parameters and local variables from local buffer overrun (turned off op)*/
#endif /* XDMF_DEBUG */

/* Compiler Optimizations will result in an 'internal compiler error', so turn them off */
#pragma optimize("g", off)
#pragma warning( disable : 4297 ) /* __declspec(nothrow), throw(), noexcept(true), or noexcept was specified in the function */
#pragma warning( disable : 4800 ) /* 'int': forcing value to bool 'true' or 'false' (performance warning) */
#pragma warning( disable : 4521 ) /* multiple copy constructors */
#else /* _WIN32 */

/* We don't need to export/import since there are no dlls */
#define XDMFCORE_EXPORT
#define XDMFCORE_TEMPLATE

#endif /* _WIN32 */

#endif /* _XDMFCORE_HPP */
#endif /*_XDMF_HPP */
