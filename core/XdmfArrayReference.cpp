/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfFunction.cpp                                                    */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/


#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfArrayReference.hpp"
#include <stack>
#include <math.h>
#include <boost/assign.hpp>
#include "XdmfError.hpp"

XdmfArrayReference::XdmfArrayReference():
  mConstructedType("")
{
}

XdmfArrayReference::~XdmfArrayReference()
{
}

std::map<std::string, std::string>
XdmfArrayReference::getConstructedProperties()
{
  return mConstructedProperties;
}

std::string
XdmfArrayReference::getConstructedType() const
{
  if (mConstructedType.c_str() != NULL) {
    return mConstructedType;
  }
  else {
    return "";
  }
}

void
XdmfArrayReference::setConstructedProperties(std::map<std::string, std::string> newProperties)
{
  mConstructedProperties = newProperties;
}

void
XdmfArrayReference::setConstructedType(std::string newType)
{
  mConstructedType = newType;
}
