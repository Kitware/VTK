/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttributeType.cpp                                               */
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
#include "XdmfAttributeType.hpp"
#include "XdmfError.hpp"

// Supported XdmfAttributeTypes
shared_ptr<const XdmfAttributeType>
XdmfAttributeType::NoAttributeType()
{
  static shared_ptr<const XdmfAttributeType> 
    p(new XdmfAttributeType("None"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::Scalar()
{
  static shared_ptr<const XdmfAttributeType> 
    p(new XdmfAttributeType("Scalar"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::Vector()
{
  static shared_ptr<const XdmfAttributeType> 
    p(new XdmfAttributeType("Vector"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::Tensor()
{
  static shared_ptr<const XdmfAttributeType>
    p(new XdmfAttributeType("Tensor"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::Matrix()
{
  static shared_ptr<const XdmfAttributeType>
    p(new XdmfAttributeType("Matrix"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::Tensor6()
{
  static shared_ptr<const XdmfAttributeType>
    p(new XdmfAttributeType("Tensor6"));
  return p;
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::GlobalId()
{
  static shared_ptr<const XdmfAttributeType>
    p(new XdmfAttributeType("GlobalId"));
  return p;
}

XdmfAttributeType::XdmfAttributeType(const std::string & name) :
  mName(name)
{
}

XdmfAttributeType::~XdmfAttributeType()
{
}

shared_ptr<const XdmfAttributeType>
XdmfAttributeType::New(const std::map<std::string, std::string> & itemProperties)
{
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("AttributeType");
  }
  if(type == itemProperties.end()) {
    // to support old xdmf defaults, return Scalar()
    return Scalar();
  }
  const std::string & typeVal = type->second;

  if(typeVal.compare("Scalar") == 0) {
    return Scalar();
  }
  else if(typeVal.compare("Vector") == 0) {
    return Vector();
  }
  else if(typeVal.compare("Tensor") == 0) {
    return Tensor();
  }
  else if(typeVal.compare("Matrix") == 0) {
    return Matrix();
  }
  else if(typeVal.compare("Tensor6") == 0) {
    return Tensor6();
  }
  else if(typeVal.compare("GlobalId") == 0) {
    return GlobalId();
  }
  else if(typeVal.compare("None") == 0) {
    return NoAttributeType();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "Type not of 'None','Scalar','Vector','Tensor', "
                     "'Matrix','Tensor6', or 'GlobalId' in "
                     "XdmfAttributeType::New");

  // unreachable
  return shared_ptr<const XdmfAttributeType>();
}

void
XdmfAttributeType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", mName));
}
