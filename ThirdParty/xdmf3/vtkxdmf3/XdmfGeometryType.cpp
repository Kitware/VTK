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
#include "string.h"

#include <sstream>

std::map<std::string, shared_ptr<const XdmfGeometryType>(*)()> XdmfGeometryType::mGeometryDefinitions;

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

shared_ptr<const XdmfGeometryType>
XdmfGeometryType::Polar()
{
  static shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("Polar", 2));
  return p;
}

shared_ptr<const XdmfGeometryType>
XdmfGeometryType::Spherical()
{
  static shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("Spherical", 3));
  return p;
}

void
XdmfGeometryType::InitTypes()
{
  mGeometryDefinitions["NONE"] = NoGeometryType;
  mGeometryDefinitions["XYZ"] = XYZ;
  mGeometryDefinitions["XY"] = XY;
  mGeometryDefinitions["POLAR"] = Polar;
  mGeometryDefinitions["SPHERICAL"] = Spherical;
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
  InitTypes();

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

  const std::string & typeVal = ConvertToUpper(type->second);

  std::map<std::string, shared_ptr<const XdmfGeometryType>(*)()>::const_iterator returnType 
    = mGeometryDefinitions.find(typeVal);

  if (returnType == mGeometryDefinitions.end()) {
    XdmfError::message(XdmfError::FATAL, "Type "
                     + typeVal + " not Supported "
                     "in XdmfGeometryType::New");
  }
  else {
    return (*(returnType->second))();
  }

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

// C Wrappers

int XdmfGeometryTypeNoGeometryType()
{
  return XDMF_GEOMETRY_TYPE_NO_GEOMETRY_TYPE;
}

int XdmfGeometryTypeXYZ()
{
  return XDMF_GEOMETRY_TYPE_XYZ;
}

int XdmfGeometryTypeXY()
{
  return XDMF_GEOMETRY_TYPE_XY;
}

int XdmfGeometryTypePolar()
{
  return XDMF_GEOMETRY_TYPE_POLAR;
}

int XdmfGeometryTypeSpherical()
{
  return XDMF_GEOMETRY_TYPE_SPHERICAL;
}

unsigned int XdmfGeometryTypeGetDimensions(int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (type) {
    case XDMF_GEOMETRY_TYPE_NO_GEOMETRY_TYPE:
      return XdmfGeometryType::NoGeometryType()->getDimensions();
      break;
    case XDMF_GEOMETRY_TYPE_XYZ:
      return XdmfGeometryType::XYZ()->getDimensions();
      break;
    case XDMF_GEOMETRY_TYPE_XY:
      return XdmfGeometryType::XY()->getDimensions();
      break;
    case XDMF_GEOMETRY_TYPE_POLAR:
      return XdmfGeometryType::Polar()->getDimensions();
      break;
    case XDMF_GEOMETRY_TYPE_SPHERICAL:
      return XdmfGeometryType::Spherical()->getDimensions();
      break;
    default:
      try {
        std::stringstream sstr;
        sstr << "Error: Invalid Geometry Type: Code " << type;
        XdmfError::message(XdmfError::FATAL, sstr.str());
      }
      catch (XdmfError & e) {
        throw e;
      }
      break;
  }
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

char * XdmfGeometryTypeGetName(int type)
{
  switch (type) {
    case XDMF_GEOMETRY_TYPE_NO_GEOMETRY_TYPE:
    {
      char * returnPointer = strdup(XdmfGeometryType::NoGeometryType()->getName().c_str());
      return returnPointer;
      break;
    }
    case XDMF_GEOMETRY_TYPE_XYZ:
    {
      char * returnPointer = strdup(XdmfGeometryType::XYZ()->getName().c_str());
      return returnPointer;
      break;
    }
    case XDMF_GEOMETRY_TYPE_XY:
    {
      char * returnPointer = strdup(XdmfGeometryType::XY()->getName().c_str());
      return returnPointer;
      break;
    }
    case XDMF_GEOMETRY_TYPE_POLAR:
    {
      char * returnPointer = strdup(XdmfGeometryType::Polar()->getName().c_str());
      return returnPointer;
      break;
    }
    case XDMF_GEOMETRY_TYPE_SPHERICAL:
    {
      char * returnPointer = strdup(XdmfGeometryType::Spherical()->getName().c_str());
      return returnPointer;
      break;
    }
    default:
    {
      char * returnPointer = NULL;
      return returnPointer;
      break;
    }
  }
}
