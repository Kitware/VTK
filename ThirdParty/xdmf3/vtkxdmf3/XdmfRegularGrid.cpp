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
class XdmfRegularGrid::XdmfRegularGridImpl : public XdmfGridImpl {

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

    bool isInitialized() const
    {
      return true;
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
        collectedProperties["Type"] = "ORIGIN_DISPLACEMENT";
//        XdmfError::message(XdmfError::FATAL, "Dimensions not 2 or 3 in XdmfGeometryTypeRegular::getProperties");
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

    bool isInitialized() const
    {
      return true;
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
      return calculateHypercubeNumElements(mRegularGrid->getDimensions()->getSize(), 1);
    }

    unsigned int
    getFacesPerElement() const
    {
      return calculateHypercubeNumElements(mRegularGrid->getDimensions()->getSize(), 2);
    }

    unsigned int
    getNodesPerElement() const
    {
      return calculateHypercubeNumElements(mRegularGrid->getDimensions()->getSize(), 0);
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
        // If not 2 or 3 just mark it as a mesh of unknown dims
        collectedProperties["Type"] = "CoRectMesh";
//        XdmfError::message(XdmfError::FATAL, 
//                           "Dimensions not 2 or 3 in "
//                           "XdmfTopologyTypeRegular::getProperties");
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
    mGridType = "Regular";
  }

  XdmfGridImpl * duplicate()
  {
    return new XdmfRegularGridImpl(mBrickSize, mDimensions, mOrigin);
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
           XdmfRegularGridImpl::XdmfTopologyRegular::New(this))
{
  mImpl = new XdmfRegularGridImpl(brickSize, numPoints, origin);
}

XdmfRegularGrid::XdmfRegularGrid(XdmfRegularGrid & refGrid) :
  XdmfGrid(refGrid)
{
  mGeometry = XdmfRegularGridImpl::XdmfGeometryRegular::New(this);
  mTopology = XdmfRegularGridImpl::XdmfTopologyRegular::New(this);  
}

XdmfRegularGrid::~XdmfRegularGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfRegularGrid::ItemTag = "Grid";

void
XdmfRegularGrid::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  XdmfGrid::copyGrid(sourceGrid); 
  if (shared_ptr<XdmfRegularGrid> classedGrid = shared_dynamic_cast<XdmfRegularGrid>(sourceGrid))
  { 
    // Copy stucture from read grid to this grid
    this->setOrigin(classedGrid->getOrigin());
    this->setDimensions(classedGrid->getDimensions());
    this->setBrickSize(classedGrid->getBrickSize());
  }
}

shared_ptr<XdmfArray>
XdmfRegularGrid::getBrickSize()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRegularGrid &>(*this).getBrickSize());
}

shared_ptr<const XdmfArray>
XdmfRegularGrid::getBrickSize() const
{
  return ((XdmfRegularGridImpl *)mImpl)->mBrickSize;
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
  return ((XdmfRegularGridImpl *)mImpl)->mDimensions;
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
  return ((XdmfRegularGridImpl *)mImpl)->mOrigin;
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
        ((XdmfRegularGridImpl *)mImpl)->mBrickSize = regularGrid->getBrickSize();
      }

      if(regularGrid->getDimensions()) {
        ((XdmfRegularGridImpl *)mImpl)->mDimensions = regularGrid->getDimensions();
      }

      if(regularGrid->getOrigin()) {
        ((XdmfRegularGridImpl *)mImpl)->mOrigin = regularGrid->getOrigin();
      }
    }
  }
}

void
XdmfRegularGrid::read()
{
  if (mGridController)
  {
    if (shared_ptr<XdmfRegularGrid> grid = shared_dynamic_cast<XdmfRegularGrid>(mGridController->read()))
    { 
      // Copy stucture from read grid to this grid
      copyGrid(grid);
    }
    else if (shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(mGridController->read()))
    {
      XdmfError::message(XdmfError::FATAL, "Error: Grid Type Mismatch");
    }
    else
    {
      XdmfError::message(XdmfError::FATAL, "Error: Invalid Grid Reference");
    }
  }
}

void
XdmfRegularGrid::release()
{
  XdmfGrid::release();
  this->setOrigin(shared_ptr<XdmfArray>());
  this->setDimensions(shared_ptr<XdmfArray>());
  this->setBrickSize(shared_ptr<XdmfArray>());
}

void
XdmfRegularGrid::setBrickSize(const shared_ptr<XdmfArray> brickSize)
{
  ((XdmfRegularGridImpl *)mImpl)->mBrickSize = brickSize;
  this->setIsChanged(true);
}

void
XdmfRegularGrid::setDimensions(const shared_ptr<XdmfArray> dimensions)
{
  ((XdmfRegularGridImpl *)mImpl)->mDimensions = dimensions;
  this->setIsChanged(true);
}

void
XdmfRegularGrid::setOrigin(const shared_ptr<XdmfArray> origin)
{
  ((XdmfRegularGridImpl *)mImpl)->mOrigin = origin;
  this->setIsChanged(true);
}

// C Wrappers

XDMFREGULARGRID * XdmfRegularGridNew2D(double xBrickSize,
                                       double yBrickSize,
                                       unsigned int xNumPoints,
                                       unsigned int yNumPoints,
                                       double xOrigin,
                                       double yOrigin)
{
  try
  {
    shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(xBrickSize,
                                                                     yBrickSize,
                                                                     xNumPoints,
                                                                     yNumPoints,
                                                                     xOrigin,
                                                                     yOrigin);
    return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
  }
  catch (...)
  {
    shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(xBrickSize,
                                                                     yBrickSize,
                                                                     xNumPoints,
                                                                     yNumPoints,
                                                                     xOrigin,
                                                                     yOrigin);
    return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
  }
}

XDMFREGULARGRID * XdmfRegularGridNew3D(double xBrickSize,
                                       double yBrickSize,
                                       double zBrickSize,
                                       unsigned int xNumPoints,
                                       unsigned int yNumPoints,
                                       unsigned int zNumPoints,
                                       double xOrigin,
                                       double yOrigin,
                                       double zOrigin)
{
  try
  {
    shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(xBrickSize,
                                                                     yBrickSize,
                                                                     zBrickSize,
                                                                     xNumPoints,
                                                                     yNumPoints,
                                                                     zNumPoints,
                                                                     xOrigin,
                                                                     yOrigin,
                                                                     zOrigin);
    return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
  }
  catch (...)
  {
    shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(xBrickSize,
                                                                     yBrickSize,
                                                                     zBrickSize,
                                                                     xNumPoints,
                                                                     yNumPoints,
                                                                     zNumPoints,
                                                                     xOrigin,
                                                                     yOrigin,
                                                                     zOrigin);
    return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
  }
}

XDMFREGULARGRID * XdmfRegularGridNew(XDMFARRAY * brickSize,
                                     XDMFARRAY * numPoints,
                                     XDMFARRAY * origin,
                                     int passControl)
{
  try
  {
    if (passControl) {
      shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(shared_ptr<XdmfArray>((XdmfArray *)brickSize),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)numPoints),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)origin));
      return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(shared_ptr<XdmfArray>((XdmfArray *)brickSize, XdmfNullDeleter()),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)numPoints, XdmfNullDeleter()),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)origin, XdmfNullDeleter()));
      return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
    }
  }
  catch (...)
  {
    if (passControl) {
      shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(shared_ptr<XdmfArray>((XdmfArray *)brickSize),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)numPoints),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)origin));
      return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRegularGrid> generatedGrid = XdmfRegularGrid::New(shared_ptr<XdmfArray>((XdmfArray *)brickSize, XdmfNullDeleter()),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)numPoints, XdmfNullDeleter()),
                                                                       shared_ptr<XdmfArray>((XdmfArray *)origin, XdmfNullDeleter()));
      return (XDMFREGULARGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*generatedGrid.get()))));
    }
  }
}

XDMFARRAY * XdmfRegularGridGetBrickSize(XDMFREGULARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  shared_ptr<XdmfArray> generatedBrick = gridPointer->getBrickSize();
  return (XDMFARRAY *)((void *)(generatedBrick.get()));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFARRAY * XdmfRegularGridGetDimensions(XDMFREGULARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  shared_ptr<XdmfArray> generatedDimensions = gridPointer->getDimensions();
  return (XDMFARRAY *)((void *)(generatedDimensions.get()));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFARRAY * XdmfRegularGridGetOrigin(XDMFREGULARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  shared_ptr<XdmfArray> generatedOrigin = gridPointer->getOrigin();
  return (XDMFARRAY *)((void *)(generatedOrigin.get()));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void XdmfRegularGridSetBrickSize(XDMFREGULARGRID * grid, XDMFARRAY * brickSize, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  if (passControl) {
    gridPointer->setBrickSize(shared_ptr<XdmfArray>((XdmfArray *)brickSize));
  }
  else {
    gridPointer->setBrickSize(shared_ptr<XdmfArray>((XdmfArray *)brickSize, XdmfNullDeleter()));
  }
  XDMF_ERROR_WRAP_END(status)
}

void XdmfRegularGridSetDimensions(XDMFREGULARGRID * grid, XDMFARRAY * dimensions, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  if (passControl) {
    gridPointer->setDimensions(shared_ptr<XdmfArray>((XdmfArray *)dimensions));
  }
  else {
    gridPointer->setDimensions(shared_ptr<XdmfArray>((XdmfArray *)dimensions, XdmfNullDeleter()));
  }
  XDMF_ERROR_WRAP_END(status)
}

void XdmfRegularGridSetOrigin(XDMFREGULARGRID * grid, XDMFARRAY * origin, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRegularGrid * gridPointer = dynamic_cast<XdmfRegularGrid *>(classedPointer);
  if (passControl) {
    gridPointer->setOrigin(shared_ptr<XdmfArray>((XdmfArray *)origin));
  }
  else {
    gridPointer->setOrigin(shared_ptr<XdmfArray>((XdmfArray *)origin, XdmfNullDeleter()));
  }
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfRegularGrid, XDMFREGULARGRID)
XDMF_GRID_C_CHILD_WRAPPER(XdmfRegularGrid, XDMFREGULARGRID)
