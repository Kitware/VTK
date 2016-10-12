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
#include <boost/tokenizer.hpp>

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

XdmfGeometry::XdmfGeometry(XdmfGeometry & refGeometry) :
  XdmfArray(refGeometry),
  mType(refGeometry.mType),
  mOrigin(refGeometry.mOrigin)
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
  std::stringstream originstream;
  for (unsigned int i = 0; i < mOrigin.size(); ++i) {
    originstream << mOrigin[i];
    if (i + 1 < mOrigin.size()) {
      originstream << " ";
    }
  }
  geometryProperties["Origin"] = originstream.str();
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
  else {
    return this->getSize() / mType->getDimensions();
  }
}

std::vector<double>
XdmfGeometry::getOrigin() const
{
  std::vector<double> returnVector(mOrigin);
  return returnVector;
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

  std::map<std::string, std::string>::const_iterator origin =
    itemProperties.find("Origin");
  if (origin != itemProperties.end()) {
    boost::tokenizer<> tokens(origin->second);
    for(boost::tokenizer<>::const_iterator iter = tokens.begin();
        iter != tokens.end();
        ++iter) {
      mOrigin.push_back(atof((*iter).c_str()));
    }
  }
}

void
XdmfGeometry::setOrigin(double newX, double newY, double newZ)
{
  mOrigin.clear();
  mOrigin.push_back(newX);
  mOrigin.push_back(newY);
  mOrigin.push_back(newZ);
  this->setIsChanged(true);
}

void
XdmfGeometry::setOrigin(std::vector<double> newOrigin)
{
  mOrigin.clear();
  for (unsigned int i = 0; i < newOrigin.size(); ++i) {
    mOrigin.push_back(newOrigin[i]);
  }
  this->setIsChanged(true);
}

void
XdmfGeometry::setType(const shared_ptr<const XdmfGeometryType> type)
{
  mType = type;
  this->setIsChanged(true);
}

// C Wrappers

XDMFGEOMETRY * XdmfGeometryNew()
{
  try
  {
    shared_ptr<XdmfGeometry> generatedGeometry = XdmfGeometry::New();
    return (XDMFGEOMETRY *)((void *)(new XdmfGeometry(*generatedGeometry.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfGeometry> generatedGeometry = XdmfGeometry::New();
    return (XDMFGEOMETRY *)((void *)(new XdmfGeometry(*generatedGeometry.get())));
  }
}

unsigned int XdmfGeometryGetNumberPoints(XDMFGEOMETRY * geometry)
{
  return ((XdmfGeometry *) geometry)->getNumberPoints();
}

double *
XdmfGeometryGetOrigin(XDMFGEOMETRY * geometry)
{
  try
  {
    std::vector<double> tempVector = ((XdmfGeometry *)(geometry))->getOrigin();
    unsigned int returnSize = tempVector.size();
    double * returnArray = new double[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
  catch (...)
  {
    std::vector<double> tempVector = ((XdmfGeometry *)(geometry))->getOrigin();
    unsigned int returnSize = tempVector.size();
    double * returnArray = new double[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
}

int
XdmfGeometryGetOriginSize(XDMFGEOMETRY * geometry)
{
  return ((XdmfGeometry *) geometry)->getOrigin().size();
}

int XdmfGeometryGetType(XDMFGEOMETRY * geometry)
{
  if (((XdmfGeometry *) geometry)->getType() == XdmfGeometryType::NoGeometryType()) {
    return XDMF_GEOMETRY_TYPE_NO_GEOMETRY_TYPE;
  }
  else if (((XdmfGeometry *) geometry)->getType() == XdmfGeometryType::XYZ()) {
    return XDMF_GEOMETRY_TYPE_XYZ;
  }
  else if (((XdmfGeometry *) geometry)->getType() == XdmfGeometryType::XY()) {
    return XDMF_GEOMETRY_TYPE_XY;
  }
  else if (((XdmfGeometry *) geometry)->getType() == XdmfGeometryType::Polar()) {
    return XDMF_GEOMETRY_TYPE_POLAR;
  }
  else if (((XdmfGeometry *) geometry)->getType() == XdmfGeometryType::Spherical()) {
    return XDMF_GEOMETRY_TYPE_SPHERICAL;
  }
  else {
    return -1;
  }
}

void
XdmfGeometrySetOrigin(XDMFGEOMETRY * geometry, double newX, double newY, double newZ)
{
  ((XdmfGeometry *) geometry)->setOrigin(newX, newY, newZ);
}

void
XdmfGeometrySetOriginArray(XDMFGEOMETRY * geometry, double * originVals, unsigned int numDims)
{
  std::vector<double> originVector;
  for (unsigned int i = 0; i < numDims; ++i)
  {
    originVector.push_back(originVals[i]);
  }
  ((XdmfGeometry *) geometry)->setOrigin(originVector);
}

void XdmfGeometrySetType(XDMFGEOMETRY * geometry, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (type) {
    case XDMF_GEOMETRY_TYPE_NO_GEOMETRY_TYPE:
      ((XdmfGeometry *) geometry)->setType(XdmfGeometryType::NoGeometryType());
      break;
    case XDMF_GEOMETRY_TYPE_XYZ:
      ((XdmfGeometry *) geometry)->setType(XdmfGeometryType::XYZ());
      break;
    case XDMF_GEOMETRY_TYPE_XY:
      ((XdmfGeometry *) geometry)->setType(XdmfGeometryType::XY());
      break;
    case XDMF_GEOMETRY_TYPE_POLAR:
      ((XdmfGeometry *) geometry)->setType(XdmfGeometryType::Polar());
      break;
    case XDMF_GEOMETRY_TYPE_SPHERICAL:
      ((XdmfGeometry *) geometry)->setType(XdmfGeometryType::Spherical());
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Geometry Type: Code " + type);
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGeometry, XDMFGEOMETRY)
XDMF_ARRAY_C_CHILD_WRAPPER(XdmfGeometry, XDMFGEOMETRY)
