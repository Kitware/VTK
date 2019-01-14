/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSetType.cpp                                                     */
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
#include "XdmfSetType.hpp"
#include "XdmfError.hpp"

std::map<std::string, shared_ptr<const XdmfSetType>(*)()> XdmfSetType::mSetDefinitions;

// Supported XdmfSetTypes
shared_ptr<const XdmfSetType>
XdmfSetType::NoSetType()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("None"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Node()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Node"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Cell()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Cell"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Face()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Face"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Edge()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Edge"));
  return p;
}

void
XdmfSetType::InitTypes()
{
  mSetDefinitions["NONE"] = NoSetType;
  mSetDefinitions["NODE"] = Node;
  mSetDefinitions["CELL"] = Cell;
  mSetDefinitions["FACE"] = Face;
  mSetDefinitions["EDGE"] = Edge;
}

XdmfSetType::XdmfSetType(const std::string & name) :
  mName(name)
{
}

XdmfSetType::~XdmfSetType()
{
}

shared_ptr<const XdmfSetType>
XdmfSetType::New(const std::map<std::string, std::string> & itemProperties)
{
  InitTypes();

  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("SetType");
  }
  if(type == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "Neither 'Type' nor 'SetType' found in itemProperties "
                       "in XdmfSetType::New");
  }
  const std::string & typeVal = ConvertToUpper(type->second);

  std::map<std::string, shared_ptr<const XdmfSetType>(*)()>::const_iterator returnType
    = mSetDefinitions.find(typeVal);

  if (returnType == mSetDefinitions.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "Type not of 'None', 'Node', 'Cell', 'Face', or "
                       "'Edge' in XdmfSetType::New");
  }
  else {
    return (*(returnType->second))();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "Type not of 'None', 'Node', 'Cell', 'Face', or "
                     "'Edge' in XdmfSetType::New");

  // unreachable
  return shared_ptr<const XdmfSetType>();
}

void
XdmfSetType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", mName));
}

// C Wrappers

int XdmfSetTypeNoSetType()
{
  return XDMF_SET_TYPE_NO_SET_TYPE;
}

int XdmfSetTypeNode()
{
  return XDMF_SET_TYPE_NODE;
}

int XdmfSetTypeCell()
{
  return XDMF_SET_TYPE_CELL;
}

int XdmfSetTypeFace()
{
  return XDMF_SET_TYPE_FACE;
}

int XdmfSetTypeEdge()
{
  return XDMF_SET_TYPE_EDGE;
}
