/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : Xdmf.hpp                                                            */
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
#define _XDMF_HPP

/*! \mainpage XDMF API
*
* \section intro Introduction
*
* The eXtensible Data Model and Format (XDMF) is a distributed data
* hub for accessing scientific data in High Performance Computing
* (HPC) applications. XDMF defines a data model and format as well as
* facilities for accessing the data in a distributed environment.
*
* XDMF differs from other data model and format efforts in that the
* "light data" is logically (and possibly physically) separated from
* the "heavy data". Light data is considered to be both "data about
* the data" such as dimensions and name, as well as small quantities
* of computed data. Heavy data is considered to be large amounts of
* data. For example, in a three dimensional structural mechanics
* calculation, the size and dimensions of the computational grid are
* light data while the actual X, Y, and Z values for the grid are
* heavy data. Calculated values like "Pressure at a node" are heavy,
* while "Total Residual Mass" for the entire calculation is light.
* Light data is stored on disk in a machine parsable language like
* XML. Heavy data is stored in a format suitable for large amounts of
* data like HDF5.
*
* While use of the XDMF API is not necessary to produce or consume
* valid datasets, it is extremely useful for handling the wide variety
* of files that are possible and its use is highly recommended. The
* XDMF API is written in C++ and is wrapped for access from other
* languages including Python and Java.
*
* XDMF utilizes reference counting shared pointers to handle ownership
* of XDMF objects. This allows multiple objects to reference a single
* XDMF object. An object is deleted and memory is reclaimed when no
* other XDMF objects hold a reference to the object. This allows
* flexibility in constructing XDMF structures, as simple structures
* can be shared instead of copied.
*
* All XDMF objects are constructed by calling New(), which returns a
* shared pointer to a newly constructed object. All default
* constructors in the XDMF API are protected, ensuring that only
* shared pointers can be created. These pointers are freed
* automatically by the shared pointer reference counting mechanism.
*
*
* Structure:
*
* Xdmf2 is structured in a tree format with an XdmfDomain serving
* as the base. The Domain contains multiple grid collections or
* grids; each with their own geometries, topologies, attributes,
* and/or sets. With the inclusion of shared pointers in Xdmf2
* a topology could be shared across multiple grids or a grid
* could be included in multiple grid collections and/or the domain.
*
* Comparing objects is done by comparing pointer addresses,
* a deep copy will not produce an equivalent object.
*
*
* C++ Examples:
*
* \subpage cppwrite "C++ Example of Xdmf Creation"
*
* \subpage cppread "C++ Example of Reading Xdmf"
*
* \subpage cppedit "C++ Example of Reading and Modifying Xdmf"
*
* Python Examples:
*
* \subpage pywrite "Python Example of Xdmf Creation"
*
* \subpage pyread "Python Example of Reading Xdmf"
*
* \subpage pyedit "Python Example of Reading and Modifying Xdmf"
*
*/

/*!
* \page cppwrite Example of Xdmf Creation
* \include ExampleXdmfWrite.cpp
*/

/*!
* \page cppread Example of Reading Xdmf
* \include ExampleXdmfRead.cpp
*/

/*!
* \page cppedit Example of Reading and Modifying
* \include ExampleXdmfEdit.cpp
*/

/*!
* \page pywrite Example of Xdmf Creation
* \include XdmfExampleWrite.py
*/

/*!
* \page pyread Example of Reading Xdmf
* \include XdmfExampleRead.py
*/

/*!
* \page pyedit Example of Reading and Modifying
* \include XdmfExampleEdit.py
*/

#include "XdmfConfig.hpp"


/* Keep all our Win32 Conversions here */
#ifdef _WIN32
#ifdef XDMFSTATIC
# define XDMFCORE_EXPORT
# define XDMFDSM_EXPORT
# define XDMF_EXPORT
# define XDMFCORE_TEMPLATE
# define XDMFDSM_TEMPLATE
# define XDMF_TEMPLATE
#else
/* Used to export/import from the dlls */
# undef XDMFCORE_EXPORT
# define XDMFCORE_EXPORT __declspec(dllimport)
# undef XDMFCORE_TEMPLATE
# define XDMFCORE_TEMPLATE extern

# undef XDMFDSM_EXPORT
# define XDMFDSM_EXPORT __declspec(dllimport)
# undef XDMFDSM_TEMPLATE
# define XDMFDSM_TEMPLATE extern

# undef XDMFUTILS_EXPORT
# define XDMFUTILS_EXPORT __declspec(dllimport)
# undef XDMFUTILS_TEMPLATE
# define XDMFUTILS_TEMPLATE extern

# ifdef XDMF_EXPORTS
# define XDMF_EXPORT __declspec(dllexport)
# define XDMF_TEMPLATE
# else /* XDMF_EXPORTS */
# define XDMF_EXPORT __declspec(dllimport)
# define XDMF_TEMPLATE extern
# endif /* XDMF_EXPORTS */
#endif /* XDMFSTATIC */

/* Compiler Warnings */
#ifndef XDMF_DEBUG
#pragma warning( disable : 4231 ) /* nonstandard extension used : 'extern' before template explicit instantiation */
#pragma warning( disable : 4251 ) /* needs to have dll-interface to be used by clients (Most of these guys are in private */
#pragma warning( disable : 4275 ) /* non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class */
#pragma warning( disable : 4373 ) /* virtual function overrides,  parameters only differed by const/volatile qualifiers */
#pragma warning( disable : 4748 ) /* /GS can not protect parameters and local variables from local buffer overrun (turned off op)*/
#endif /* XDMF_DEBUG */

/* Compiler Optimizations will result in an 'internal compiler error', so turn them off */
#pragma optimize("g", off)

#pragma warning( disable : 4297 ) /* __declspec(nothrow), throw(), noexcept(true), or noexcept was specified in the function */
#pragma warning( disable : 4800 ) /* 'int': forcing value to bool 'true' or 'false' (performance warning) */
#pragma warning( disable : 4250 ) /* inherits insert via dominance */
#pragma warning( disable : 4521 ) /* multiple copy constructors */

#else /* _WIN32 */
/* We don't need to export/import since there are no dlls */
#define XDMFCORE_EXPORT
#define XDMFDSM_EXPORT
#define XDMF_EXPORT
#define XDMFCORE_TEMPLATE
#define XDMFDSM_TEMPLATE
#define XDMF_TEMPLATE
#endif /* _WIN32 */
#endif /* _XDMF_HPP */
