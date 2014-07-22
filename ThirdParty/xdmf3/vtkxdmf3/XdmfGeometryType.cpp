/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGeometryType.cpp                                                */
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

#include <utility>
#include "XdmfGeometryType.hpp"
#include "XdmfError.hpp"

// Supported XdmfGeometryTypes
shared_ptr<const XdmfGeometryType>
XdmfGeometryType::NoGeometryType()
{
  static shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("None", 0));
  return p;
}

shared_ptr<const XdmfGeometryType>
XdmfGeometryType::XYZ()
{
  static shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("XYZ", 3));
  return p;
}

shared_ptr<const XdmfGeometryType>
XdmfGeometryType::XY()
{
  static shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("XY", 2));
  return p;
}

XdmfGeometryType::XdmfGeometryType(const std::string& name,
                                   const int& dimensions) :
  mDimensions(dimensions),
  mName(name)
{
}

XdmfGeometryType::~XdmfGeometryType()
{
}

shared_ptr<const XdmfGeometryType>
XdmfGeometryType::New(const std::map<std::string, std::string> & itemProperties)
{
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("GeometryType");
  }
  if(type == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "Neither 'Type' nor 'GeometryType' in itemProperties "
                       "in XdmfGeometryType::New");
  }
  const std::string & typeVal = type->second;

  if(typeVal.compare("None") == 0) {
    return NoGeometryType();
  }
  else if(typeVal.compare("XYZ") == 0) {
    return XYZ();
  }
  else if(typeVal.compare("XY") == 0) {
    return XY();
  }

  XdmfError::message(XdmfError::FATAL, "Type not 'None', 'XYZ', or 'XY' "
                     "in XdmfGeometryType::New");

  return shared_ptr<const XdmfGeometryType>();
}

unsigned int
XdmfGeometryType::getDimensions() const
{
  return mDimensions;
}

std::string
XdmfGeometryType::getName() const
{
  return mName;
}

void
XdmfGeometryType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", mName));
}
