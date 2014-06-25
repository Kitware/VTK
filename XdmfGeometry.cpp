/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGeometry.cpp                                                    */
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

#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"

shared_ptr<XdmfGeometry> XdmfGeometry::New()
{
  shared_ptr<XdmfGeometry> p(new XdmfGeometry());
  return p;
}

XdmfGeometry::XdmfGeometry() :
  XdmfArray(),
  mNumberPoints(0),
  mType(XdmfGeometryType::NoGeometryType())
{
}

XdmfGeometry::~XdmfGeometry()
{
}

const std::string XdmfGeometry::ItemTag = "Geometry";

std::map<std::string, std::string>
XdmfGeometry::getItemProperties() const
{
  std::map<std::string, std::string> geometryProperties;
  mType->getProperties(geometryProperties);
  return geometryProperties;
}

std::string
XdmfGeometry::getItemTag() const
{
  return ItemTag;
}

unsigned int
XdmfGeometry::getNumberPoints() const
{
  if(mType->getDimensions() == 0) {
    return 0;
  }
  return this->getSize() / mType->getDimensions();
}

shared_ptr<const XdmfGeometryType>
XdmfGeometry::getType() const
{
  return mType;
}

void
XdmfGeometry::populateItem(const std::map<std::string, std::string> & itemProperties,
                           const std::vector<shared_ptr<XdmfItem> > & childItems,
                           const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("GeometryType");
  }

  if (type != itemProperties.end()) {
    if(type->second.compare("X_Y_Z") == 0) {
      mType = XdmfGeometryType::XYZ();

      // Building Function equivalent
      std::vector<std::string> dimensionIDVector;
      dimensionIDVector.push_back("X");
      dimensionIDVector.push_back("Y");
      dimensionIDVector.push_back("Z");

      std::map<std::string, shared_ptr<XdmfArray> > dimensionMap;

      unsigned int dimensionIDIndex = 0;

      // Find X, Y, and Z Arrays
      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end() && dimensionIDIndex < dimensionIDVector.size();
          ++iter) {
        if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
          dimensionMap[dimensionIDVector[dimensionIDIndex]] = array;
          dimensionIDIndex++;
        }
      }

      shared_ptr<XdmfFunction> geoFunction = XdmfFunction::New("X#Y#Z", dimensionMap);

      this->setReference(geoFunction);
      this->setReadMode(XdmfArray::Reference);
    }
    else if(type->second.compare("X_Y") == 0) {
      mType = XdmfGeometryType::XY();

      // Building Function equivalent
      std::vector<std::string> dimensionIDVector;
      dimensionIDVector.push_back("X");
      dimensionIDVector.push_back("Y");

      std::map<std::string, shared_ptr<XdmfArray> > dimensionMap;

      unsigned int dimensionIDIndex = 0;

      // Find X, Y, and Z Arrays
      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end() && dimensionIDIndex < dimensionIDVector.size();
          ++iter) {
        if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
          dimensionMap[dimensionIDVector[dimensionIDIndex]] = array;
          dimensionIDIndex++;
        }
      }

      shared_ptr<XdmfFunction> geoFunction = XdmfFunction::New("X#Y", dimensionMap);

      this->setReference(geoFunction);
      this->setReadMode(XdmfArray::Reference);
    }
    else {
      mType = XdmfGeometryType::New(itemProperties);
      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end();
          ++iter) {
        if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
          this->swap(array);
          if (array->getReference()) {
            this->setReference(array->getReference());
            this->setReadMode(XdmfArray::Reference);
          }
          break;
        }
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Neither 'Type' nor 'GeometryType' in itemProperties "
                       "in XdmfGeometry::populateItem");
  }
}

void
XdmfGeometry::setType(const shared_ptr<const XdmfGeometryType> type)
{
  mType = type;
}
