/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSystemUtils.cpp                                                 */
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

#include <libxml/uri.h>
#include <limits.h>
#include <stdlib.h>
#include "XdmfSystemUtils.hpp"
#include "XdmfCoreConfig.hpp"
#include <iostream>

XdmfSystemUtils::XdmfSystemUtils()
{
}

XdmfSystemUtils::~XdmfSystemUtils()
{
}

#ifdef XDMF_NO_REALPATH
//allows symbolic links
std::string
XdmfSystemUtils::getRealPath(const std::string & path)
{
  return path;
}
#else
std::string
XdmfSystemUtils::getRealPath(const std::string & path)
{
  xmlURIPtr ref = NULL;
  ref = xmlCreateURI();
  xmlParseURIReference(ref, path.c_str());
  char realPath[PATH_MAX];
  char *rp = realpath(ref->path, realPath);
  xmlFreeURI(ref);
  return std::string(rp);
}
#endif
