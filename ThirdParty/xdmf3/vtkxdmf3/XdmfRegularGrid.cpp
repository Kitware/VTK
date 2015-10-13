/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfRegularGrid.cpp                                                 */
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

#include <cmath>
#include "XdmfArray.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfError.hpp"

/**
 * PIMPL
 */
class XdmfRegularGrid::XdmfRegularGridImpl {

public:

  class XdmfGeometryRegular : public XdmfGeometry
  {

  public:

    static shared_ptr<XdmfGeometryRegular>
    New(XdmfRegularGrid * const regularGrid)
    {
      shared_ptr<XdmfGeometryRegular> p(new XdmfGeometryRegular(regularGrid));
      return p;
    }

    unsigned int
    getNumberPoints() const
    {
      const shared_ptr<const XdmfArray> dimensions = 
        mRegularGrid->getDimensions();
      if(dimensions->getSize() == 0) {
        return 0;
      }
      unsigned int toReturn = 1;
      for(unsigned int i=0; i<dimensions->getSize(); ++i) {
        toReturn *= dimensions->getValue<unsigned int>(i);
      }
      return toReturn;
    }

    void
    traverse(const shared_ptr<XdmfBaseVisitor> visitor)
    {
      shared_ptr<XdmfArray> origin = mRegularGrid->getOrigin();
      shared_ptr<XdmfArray> brickSize = mRegularGrid->getBrickSize();
      origin->accept(visitor);
      brickSize->accept(visitor);
    }

  private:

    XdmfGeometryRegular(XdmfRegularGrid * const regularGrid) :
      mRegularGrid(regularGrid)
    {
      this->setType(XdmfGeometryTypeRegular::New(mRegularGrid));
    }

    XdmfRegularGrid * const mRegularGrid;
  };

  class XdmfGeometryTypeRegular : public XdmfGeometryType
  {

  public:

    static shared_ptr<const XdmfGeometryTypeRegular>
    New(const XdmfRegularGrid * const regularGrid)
    {
      shared_ptr<const XdmfGeometryTypeRegular> 
        p(new XdmfGeometryTypeRegular(regularGrid));
      return p;
    }

    unsigned int
    getDimensions() const
    {
      return mRegularGrid->getDimensions()->getSize();
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      const unsigned int dimensions = this->getDimensions();
      if(dimensions == 3) {
        collectedProperties["Type"] = "ORIGIN_DXDYDZ";
      }
      else if(dimensions == 2) {
        collectedProperties["Type"] = "ORIGIN_DXDY";
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Dimensions not 2 or 3 in "
                           "XdmfGeometryTypeRegular::getProperties");
      }
    }

  private:

    XdmfGeometryTypeRegular(const XdmfRegularGrid * const regularGrid) :
      XdmfGeometryType("", 0),
      mRegularGrid(regularGrid)
    {
    }

    const XdmfRegularGrid * const mRegularGrid;

  };

  class XdmfTopologyRegular : public XdmfTopology
  {

  public:

    static shared_ptr<XdmfTopologyRegular>
    New(const XdmfRegularGrid * const regularGrid)
    {
      shared_ptr<XdmfTopologyRegular> p(new XdmfTopologyRegular(regularGrid));
      return p;
    }

    unsigned int
    getNumberElements() const
    {
      const shared_ptr<const XdmfArray> dimensions = 
        mRegularGrid->getDimensions();
      if(dimensions->getSize() == 0) {
        return 0;
      }
      unsigned int toReturn = 1;
      for(unsigned int i=0; i<dimensions->getSize(); ++i) {
        toReturn *= (dimensions->getValue<unsigned int>(i) - 1);
      }
      return toReturn;
    }

  private:

    XdmfTopologyRegular(const XdmfRegularGrid * const regularGrid) :
      mRegularGrid(regularGrid)
    {
      this->setType(XdmfTopologyTypeRegular::New(regularGrid));
    }

    const XdmfRegularGrid * const mRegularGrid;
  };

  class XdmfTopologyTypeRegular : public XdmfTopologyType
  {

  public:

    static shared_ptr<const XdmfTopologyTypeRegular>
    New(const XdmfRegularGrid * const regularGrid)
    {
      shared_ptr<const XdmfTopologyTypeRegular>
        p(new XdmfTopologyTypeRegular(regularGrid));
      return p;
    }

    unsigned int
    getEdgesPerElement() const
    {
      const unsigned int dimensions = mRegularGrid->getDimensions()->getSize();
      if(dimensions == 2) {
        return 4;
      }
      else if(dimensions == 3) {
        return 12;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeRegular::getEdgesPerElement");
      }
      return 0;
    }

    unsigned int
    getFacesPerElement() const
    {
      const unsigned int dimensions = mRegularGrid->getDimensions()->getSize();
      if(dimensions == 2) {
        return 1;
      }
      else if(dimensions == 3) {
        return 6;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeRegular::getFacesPerElement");
      }
      return 0;
    }

    unsigned int
    getNodesPerElement() const
    {
      // 2^Dimensions
      // e.g. 1D = 2 nodes per element and 2D = 4 nodes per element.
      return (unsigned int)
        std::pow(2, (double)mRegularGrid->getDimensions()->getSize());
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      shared_ptr<const XdmfArray> dimensions = mRegularGrid->getDimensions();
      if(dimensions->getSize() == 3){
        collectedProperties["Type"] = "3DCoRectMesh";
      }
      else if(dimensions->getSize() == 2) {
        collectedProperties["Type"] = "2DCoRectMesh";
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Dimensions not 2 or 3 in "
                           "XdmfTopologyTypeRegular::getProperties");
      }
      collectedProperties["Dimensions"] = dimensions->getValuesString();
    }

  private:

    XdmfTopologyTypeRegular(const XdmfRegularGrid * const regularGrid) :
      XdmfTopologyType(0, 0, std::vector<shared_ptr<const XdmfTopologyType> >(), 0, "foo", XdmfTopologyType::Structured, 0x1102),
      mRegularGrid(regularGrid)
    {
    }

    const XdmfRegularGrid * const mRegularGrid;

  };

  XdmfRegularGridImpl(const shared_ptr<XdmfArray> brickSize,
                      const shared_ptr<XdmfArray> numPoints,
                      const shared_ptr<XdmfArray> origin) :
    mBrickSize(brickSize),
    mDimensions(numPoints),
    mOrigin(origin)
  {
  }

  shared_ptr<XdmfArray> mBrickSize;
  shared_ptr<XdmfArray> mDimensions;
  shared_ptr<XdmfArray> mOrigin;

};

shared_ptr<XdmfRegularGrid>
XdmfRegularGrid::New(const double xBrickSize,
                     const double yBrickSize,
                     const unsigned int xNumPoints,
                     const unsigned int yNumPoints,
                     const double xOrigin,
                     const double yOrigin)
{
  shared_ptr<XdmfArray> brickSize = XdmfArray::New();
  brickSize->initialize<double>(2);
  brickSize->insert(0, xBrickSize);
  brickSize->insert(1, yBrickSize);
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->initialize<unsigned int>(2);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  shared_ptr<XdmfArray> origin = XdmfArray::New();
  origin->initialize<double>(2);
  origin->insert(0, xOrigin);
  origin->insert(1, yOrigin);
  shared_ptr<XdmfRegularGrid> p(new XdmfRegularGrid(brickSize,
                                                    numPoints,
                                                    origin));
  return p;
}

shared_ptr<XdmfRegularGrid>
XdmfRegularGrid::New(const double xBrickSize,
                     const double yBrickSize,
                     const double zBrickSize,
                     const unsigned int xNumPoints,
                     const unsigned int yNumPoints,
                     const unsigned int zNumPoints,
                     const double xOrigin,
                     const double yOrigin,
                     const double zOrigin)
{
  shared_ptr<XdmfArray> brickSize = XdmfArray::New();
  brickSize->initialize<double>(3);
  brickSize->insert(0, xBrickSize);
  brickSize->insert(1, yBrickSize);
  brickSize->insert(2, zBrickSize);
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->initialize<unsigned int>(3);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  numPoints->insert(2, zNumPoints);
  shared_ptr<XdmfArray> origin = XdmfArray::New();
  origin->initialize<double>(3);
  origin->insert(0, xOrigin);
  origin->insert(1, yOrigin);
  origin->insert(2, zOrigin);
  shared_ptr<XdmfRegularGrid> p(new XdmfRegularGrid(brickSize,
                                                    numPoints,
                                                    origin));
  return p;
}

shared_ptr<XdmfRegularGrid>
XdmfRegularGrid::New(const shared_ptr<XdmfArray> brickSize,
                     const shared_ptr<XdmfArray> numPoints,
                     const shared_ptr<XdmfArray> origin)
{
  shared_ptr<XdmfRegularGrid> p(new XdmfRegularGrid(brickSize,
                                                    numPoints,
                                                    origin));
  return p;
}

XdmfRegularGrid::XdmfRegularGrid(const shared_ptr<XdmfArray> brickSize,
                                 const shared_ptr<XdmfArray> numPoints,
                                 const shared_ptr<XdmfArray> origin) :
  XdmfGrid(XdmfRegularGridImpl::XdmfGeometryRegular::New(this),
           XdmfRegularGridImpl::XdmfTopologyRegular::New(this)),
  mImpl(new XdmfRegularGridImpl(brickSize, numPoints, origin))
{
}

XdmfRegularGrid::~XdmfRegularGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfRegularGrid::ItemTag = "Grid";

shared_ptr<XdmfArray>
XdmfRegularGrid::getBrickSize()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRegularGrid &>(*this).getBrickSize());
}

shared_ptr<const XdmfArray>
XdmfRegularGrid::getBrickSize() const
{
  return mImpl->mBrickSize;
}

shared_ptr<XdmfArray>
XdmfRegularGrid::getDimensions()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRegularGrid &>(*this).getDimensions());
}

shared_ptr<const XdmfArray>
XdmfRegularGrid::getDimensions() const
{
  return mImpl->mDimensions;
}

shared_ptr<XdmfArray>
XdmfRegularGrid::getOrigin()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRegularGrid &>(*this).getOrigin());
}

shared_ptr<const XdmfArray>
XdmfRegularGrid::getOrigin() const
{
  return mImpl->mOrigin;
}

void
XdmfRegularGrid::populateItem(const std::map<std::string, std::string> & itemProperties,
                              const std::vector<shared_ptr<XdmfItem> > & childItems,
                              const XdmfCoreReader * const reader)
{
  XdmfGrid::populateItem(itemProperties, childItems, reader);

  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfRegularGrid> regularGrid =
       shared_dynamic_cast<XdmfRegularGrid>(*iter)) {
      if(regularGrid->getBrickSize()) {
        mImpl->mBrickSize = regularGrid->getBrickSize();
      }

      if(regularGrid->getDimensions()) {
        mImpl->mDimensions = regularGrid->getDimensions();
      }

      if(regularGrid->getOrigin()) {
        mImpl->mOrigin = regularGrid->getOrigin();
      }
    }
  }
}

void
XdmfRegularGrid::setBrickSize(const shared_ptr<XdmfArray> brickSize)
{
  mImpl->mBrickSize = brickSize;
}

void
XdmfRegularGrid::setDimensions(const shared_ptr<XdmfArray> dimensions)
{
  mImpl->mDimensions = dimensions;
}

void
XdmfRegularGrid::setOrigin(const shared_ptr<XdmfArray> origin)
{
  mImpl->mOrigin = origin;
}
