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

std::map<std::string, shared_ptr<const XdmfAttributeType>(*)()> XdmfAttributeType::mAttributeDefinitions;

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

void
XdmfAttributeType::InitTypes()
{
  mAttributeDefinitions["NONE"] = NoAttributeType;
  mAttributeDefinitions["SCALAR"] = Scalar;
  mAttributeDefinitions["VECTOR"] = Vector;
  mAttributeDefinitions["TENSOR"] = Tensor;
  mAttributeDefinitions["MATRIX"] = Matrix;
  mAttributeDefinitions["TENSOR6"] = Tensor6;
  mAttributeDefinitions["GLOBALID"] = GlobalId;
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
  InitTypes();
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("AttributeType");
  }
  if(type == itemProperties.end()) {
    // to support old xdmf defaults, return Scalar()
    return Scalar();
  }


  const std::string & typeVal = ConvertToUpper(type->second);

  std::map<std::string, shared_ptr<const XdmfAttributeType>(*)()>::const_iterator returnType = mAttributeDefinitions.find(typeVal);

  if (returnType == mAttributeDefinitions.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "Type not of 'None','Scalar','Vector','Tensor', "
                       "'Matrix','Tensor6', or 'GlobalId' in "
                       "XdmfAttributeType::New");
  }
  else {
    return (*(returnType->second))();
  }

  // unreachable
  return shared_ptr<const XdmfAttributeType>();
}

void
XdmfAttributeType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", mName));
}

// C Wrappers

int XdmfAttributeTypeScalar()
{
  return XDMF_ATTRIBUTE_TYPE_SCALAR;
}

int XdmfAttributeTypeVector()
{
  return XDMF_ATTRIBUTE_TYPE_VECTOR;
}

int XdmfAttributeTypeTensor()
{
  return XDMF_ATTRIBUTE_TYPE_TENSOR;
}

int XdmfAttributeTypeMatrix()
{
  return XDMF_ATTRIBUTE_TYPE_MATRIX;
}

int XdmfAttributeTypeTensor6()
{
  return XDMF_ATTRIBUTE_TYPE_TENSOR6;
}

int XdmfAttributeTypeGlobalId()
{
  return XDMF_ATTRIBUTE_TYPE_GLOBALID;
}

int XdmfAttributeTypeNoAttributeType()
{
  return XDMF_ATTRIBUTE_TYPE_NOTYPE;
}
