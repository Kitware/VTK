/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttributeCenter.cpp                                             */
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
#include "XdmfAttributeCenter.hpp"
#include "XdmfError.hpp"

std::map<std::string, shared_ptr<const XdmfAttributeCenter>(*)()> XdmfAttributeCenter::mAttributeCenterDefinitions;

// Supported XdmfAttributeCenters
shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Grid()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Grid"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Cell()
{
  static shared_ptr<const XdmfAttributeCenter> 
    p(new XdmfAttributeCenter("Cell"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Face()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Face"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Edge()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Edge"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Node()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Node"));
  return p;
}

void
XdmfAttributeCenter::InitTypes()
{
  mAttributeCenterDefinitions["GRID"] = Grid;
  mAttributeCenterDefinitions["CELL"] = Cell;
  mAttributeCenterDefinitions["FACE"] = Face;
  mAttributeCenterDefinitions["EDGE"] = Edge;
  mAttributeCenterDefinitions["NODE"] = Node;
}

XdmfAttributeCenter::XdmfAttributeCenter(const std::string & name) :
  mName(name)
{
}

XdmfAttributeCenter::~XdmfAttributeCenter()
{
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::New(const std::map<std::string, std::string> & itemProperties)
{
  InitTypes();
  std::map<std::string, std::string>::const_iterator center =
    itemProperties.find("Center");
  if(center == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "'Center' not found in itemProperties in "
                       "XdmfAttributeCenter::New");
  }

  const std::string & centerVal = ConvertToUpper(center->second);

  std::map<std::string, shared_ptr<const XdmfAttributeCenter>(*)()>::const_iterator returnType = mAttributeCenterDefinitions.find(centerVal);

  if (returnType == mAttributeCenterDefinitions.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "Center not of 'Grid','Cell','Face','Edge','Node' "
                       "in XdmfAttributeCenter::New");
  }
  else {
    return (*(returnType->second))();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "Center not of 'Grid','Cell','Face','Edge','Node' "
                     "in XdmfAttributeCenter::New");

  // unreachable
  return shared_ptr<const XdmfAttributeCenter>();
}

void
XdmfAttributeCenter::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Center", mName));
}

// C Wrappers

int XdmfAttributeCenterGrid()
{
  return XDMF_ATTRIBUTE_CENTER_GRID;
}

int XdmfAttributeCenterCell()
{
  return XDMF_ATTRIBUTE_CENTER_CELL;
}

int XdmfAttributeCenterFace()
{
  return XDMF_ATTRIBUTE_CENTER_FACE;
}

int XdmfAttributeCenterEdge()
{
  return XDMF_ATTRIBUTE_CENTER_EDGE;
}

int XdmfAttributeCenterNode()
{
  return XDMF_ATTRIBUTE_CENTER_NODE;
}
