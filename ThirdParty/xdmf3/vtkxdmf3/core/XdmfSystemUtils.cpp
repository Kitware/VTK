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
#include "string.h"

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
#ifdef WIN32
  char realPath[_MAX_PATH];
  _fullpath(realPath, path.c_str(), _MAX_PATH);
  xmlFreeURI(ref);
  return realPath;
#else
  char realPath[PATH_MAX];
  char *rp = realpath(ref->path, realPath);
  if (rp == 0)
  {
     //indicates a failure that we are silently ignoring
     //TODO: realPath is now undefined but in practice
     //ends up path.c_str()
     rp = realPath;
  }
  xmlFreeURI(ref);
  return std::string(rp);
#endif
}
#endif

char * XdmfSystemUtilsGetRealPath(char * path)
{
  std::string returnstring = XdmfSystemUtils::getRealPath(std::string(path));
  char * returnPointer = strdup(returnstring.c_str());
  return returnPointer;
}
