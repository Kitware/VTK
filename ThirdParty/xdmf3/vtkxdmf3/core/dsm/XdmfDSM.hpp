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
#ifndef _XDMFDSM_HPP
#define _XDMFDSM_HPP
/* Keep all our Win32 Conversions here */
#ifdef _WIN32
/* Used to export/import from the dlls */
#undef XDMFCORE_EXPORT
#define XDMFCORE_EXPORT __declspec(dllimport)
#undef XDMFCORE_TEMPLATE
#define XDMFCORE_TEMPLATE extern

#ifdef XdmfDSM_EXPORTS
#define XDMFDSM_EXPORT __declspec(dllexport)
#define XDMFDSM_TEMPLATE
#else /* Xdmf_EXPORTS */
#define XDMFDSM_EXPORT __declspec(dllimport)
#define XDMFDSM_TEMPLATE extern
#endif /* Xdmf_EXPORTS */

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

#else /* _WIN32 */
/* We don't need to export/import since there are no dlls */
#define XDMFCORE_EXPORT
#define XDMFDSM_EXPORT
#define XDMFCORE_TEMPLATE
#define XDMFDSM_TEMPLATE
#endif /* _WIN32 */
#endif /* _XDMFCORE_HPP */
#endif /*_XDMF_HPP */

