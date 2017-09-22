/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCurviliniearGrid.cpp                                            */
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
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfError.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

/**
 * PIMPL
 */
class XdmfCurvilinearGrid::XdmfCurvilinearGridImpl {

public:

  class XdmfTopologyCurvilinear : public XdmfTopology
  {

  public:

    static shared_ptr<XdmfTopologyCurvilinear>
    New(const XdmfCurvilinearGrid * const curvilinearGrid)
    {
      shared_ptr<XdmfTopologyCurvilinear> 
        p(new XdmfTopologyCurvilinear(curvilinearGrid));
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
        mCurvilinearGrid->getDimensions();
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

    XdmfTopologyCurvilinear(const XdmfCurvilinearGrid * const curvilinearGrid) :
      mCurvilinearGrid(curvilinearGrid)
    {
      this->setType(XdmfTopologyTypeCurvilinear::New(curvilinearGrid));
    }

    const XdmfCurvilinearGrid * const mCurvilinearGrid;
  };

  class XdmfTopologyTypeCurvilinear : public XdmfTopologyType
  {

  public:

    static shared_ptr<const XdmfTopologyTypeCurvilinear>
    New(const XdmfCurvilinearGrid * const curvilinearGrid)
    {
      shared_ptr<const XdmfTopologyTypeCurvilinear>
        p(new XdmfTopologyTypeCurvilinear(curvilinearGrid));
      return p;
    }

    unsigned int
    getEdgesPerElement() const
    {
      return calculateHypercubeNumElements(mCurvilinearGrid->getDimensions()->getSize(), 1);
/*
      const unsigned int dimensions = 
        mCurvilinearGrid->getDimensions()->getSize();
      if (dimensions == 1) {
        return 1;
      }
      if(dimensions == 2) {
        return 4;
      }
      else if(dimensions >= 3) {
        return 12;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeCurvilinear::getEdgesPerElement");
      }
      return 0;
*/
    }

    unsigned int
    getFacesPerElement() const
    {
      return calculateHypercubeNumElements(mCurvilinearGrid->getDimensions()->getSize(), 2);
/*
      const unsigned int dimensions = 
        mCurvilinearGrid->getDimensions()->getSize();
      if (dimensions == 1) {
        return 0;
      }
      else if(dimensions == 2) {
        return 1;
      }
      else if(dimensions == 3) {
        return 6;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeCurvilinear::getFacesPerElement");
      }
      return 0;
*/
    }

    unsigned int
    getNodesPerElement() const
    {
      return calculateHypercubeNumElements(mCurvilinearGrid->getDimensions()->getSize(), 0);
      // 2^Dimensions
      // e.g. 1D = 2 nodes per element and 2D = 4 nodes per element.
//      return (unsigned int)
//        std::pow(2, (double)mCurvilinearGrid->getDimensions()->getSize());
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      shared_ptr<const XdmfArray> dimensions =
        mCurvilinearGrid->getDimensions();
      if(dimensions->getSize() == 3) {
        collectedProperties["Type"] = "3DSMesh";
      }
      else if(dimensions->getSize() == 2) {
        collectedProperties["Type"] = "2DSMesh";
      }
      else {
        collectedProperties["Type"] = "SMesh";
//        XdmfError::message(XdmfError::FATAL, 
//                           "Grid dimensions not 2 or 3 in "
//                           "XdmfTopologyTypeCurvilinear::getProperties");
      }
      collectedProperties["Dimensions"] = dimensions->getValuesString();
    }

  private:

    XdmfTopologyTypeCurvilinear(const XdmfCurvilinearGrid * const curvilinearGrid) :
      XdmfTopologyType(0,
                       0,
                       std::vector<shared_ptr<const XdmfTopologyType> >(),
                       0,
                       "foo",
                       XdmfTopologyType::Structured,
                       0x1110),
      mCurvilinearGrid(curvilinearGrid)
    {
    }

    const XdmfCurvilinearGrid * const mCurvilinearGrid;

  };

  XdmfCurvilinearGridImpl(const shared_ptr<XdmfArray> numPoints) :
    mDimensions(numPoints)
  {
  }

  shared_ptr<XdmfArray> mDimensions;

};

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const unsigned int xNumPoints,
                         const unsigned int yNumPoints)
{
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->initialize<unsigned int>(2);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const unsigned int xNumPoints,
                         const unsigned int yNumPoints,
                         const unsigned int zNumPoints)
{
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->initialize<unsigned int>(3);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  numPoints->insert(2, zNumPoints);
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const shared_ptr<XdmfArray> numPoints)
{
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

XdmfCurvilinearGrid::XdmfCurvilinearGrid(const shared_ptr<XdmfArray> numPoints) :
  XdmfGrid(XdmfGeometry::New(),
           XdmfCurvilinearGridImpl::XdmfTopologyCurvilinear::New(this)),
  mImpl(new XdmfCurvilinearGridImpl(numPoints))
{
}

XdmfCurvilinearGrid::~XdmfCurvilinearGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfCurvilinearGrid::ItemTag = "Grid";

void
XdmfCurvilinearGrid::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  XdmfGrid::copyGrid(sourceGrid);
  if (shared_ptr<XdmfCurvilinearGrid> classedGrid = 
      shared_dynamic_cast<XdmfCurvilinearGrid>(sourceGrid))
  {
    // Copy stucture from read grid to this grid
    this->setGeometry(classedGrid->getGeometry());
    this->setDimensions(classedGrid->getDimensions());
  }
}

shared_ptr<XdmfArray>
XdmfCurvilinearGrid::getDimensions()
{
  return const_pointer_cast<XdmfArray>
    (static_cast<const XdmfCurvilinearGrid &>(*this).getDimensions());
}

shared_ptr<const XdmfArray>
XdmfCurvilinearGrid::getDimensions() const
{
  return mImpl->mDimensions;
}

shared_ptr<XdmfGeometry>
XdmfCurvilinearGrid::getGeometry()
{
  return const_pointer_cast<XdmfGeometry>
    (static_cast<const XdmfGrid &>(*this).getGeometry());
}

void
XdmfCurvilinearGrid::populateItem(const std::map<std::string, std::string> & itemProperties,
                                  const std::vector<shared_ptr<XdmfItem> > & childItems,
                                  const XdmfCoreReader * const reader)
{
  XdmfGrid::populateItem(itemProperties, childItems, reader);

  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfCurvilinearGrid> curvilinearGrid =
       shared_dynamic_cast<XdmfCurvilinearGrid>(*iter)) {
      ((XdmfCurvilinearGridImpl *)mImpl)->mDimensions = curvilinearGrid->getDimensions();
    }
  }
}

void
XdmfCurvilinearGrid::read()
{
  if (mGridController)
  {
    if (shared_ptr<XdmfCurvilinearGrid> grid = 
	shared_dynamic_cast<XdmfCurvilinearGrid>(mGridController->read())) { 
      // Copy stucture from read grid to this grid
      copyGrid(grid);
    }
    else if (shared_ptr<XdmfGrid> grid = 
	     shared_dynamic_cast<XdmfGrid>(mGridController->read())) {
      XdmfError::message(XdmfError::FATAL, "Error: Grid Type Mismatch");
    }
    else {
      XdmfError::message(XdmfError::FATAL, "Error: Invalid Grid Reference");
    }
  }
}

void
XdmfCurvilinearGrid::release()
{
  XdmfGrid::release();
  this->setGeometry(shared_ptr<XdmfGeometry>());
  this->setDimensions(shared_ptr<XdmfArray>());
}

void
XdmfCurvilinearGrid::setDimensions(const shared_ptr<XdmfArray> dimensions)
{
  mImpl->mDimensions = dimensions;
  this->setIsChanged(true);
}

void
XdmfCurvilinearGrid::setGeometry(const shared_ptr<XdmfGeometry> geometry)
{
  mGeometry = geometry;
  this->setIsChanged(true);
}

// C Wrappers

XDMFCURVILINEARGRID * XdmfCurvilinearGridNew2D(unsigned int xNumPoints,
                                               unsigned int yNumPoints)
{
  shared_ptr<XdmfCurvilinearGrid> * p = 
    new shared_ptr<XdmfCurvilinearGrid>(XdmfCurvilinearGrid::New(xNumPoints,
								 yNumPoints));
  return (XDMFCURVILINEARGRID *) p;
}

XDMFCURVILINEARGRID * XdmfCurvilinearGridNew3D(unsigned int xNumPoints,
                                               unsigned int yNumPoints,
                                               unsigned int zNumPoints)
{
  shared_ptr<XdmfCurvilinearGrid> * p = 
    new shared_ptr<XdmfCurvilinearGrid>(XdmfCurvilinearGrid::New(xNumPoints,
								 yNumPoints,
								 zNumPoints));
  return (XDMFCURVILINEARGRID *) p;
}

XDMFCURVILINEARGRID * XdmfCurvilinearGridNew(XDMFARRAY * numPoints, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfArray> & refNumPoints = *(shared_ptr<XdmfArray> *)(numPoints);
  shared_ptr<XdmfCurvilinearGrid> * p = 
    new shared_ptr<XdmfCurvilinearGrid>(XdmfCurvilinearGrid::New(refNumPoints));
  return (XDMFCURVILINEARGRID *) p;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFARRAY * XdmfCurvilinearGridGetDimensions(XDMFCURVILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfCurvilinearGrid> & refGrid = *(shared_ptr<XdmfCurvilinearGrid> *)(grid);
  shared_ptr<XdmfArray> * dim = new shared_ptr<XdmfArray>(refGrid->getDimensions());
  return (XDMFARRAY *) dim;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFGEOMETRY * XdmfCurvilinearGridGetGeometry(XDMFCURVILINEARGRID * grid)
{
  shared_ptr<XdmfCurvilinearGrid> & refGrid = *(shared_ptr<XdmfCurvilinearGrid> *)(grid);
  shared_ptr<XdmfGeometry> * geometry = new shared_ptr<XdmfGeometry>(refGrid->getGeometry());
  return (XDMFGEOMETRY *) geometry;
}

void XdmfCurvilinearGridSetDimensions(XDMFCURVILINEARGRID * grid, XDMFARRAY * dimensions, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfCurvilinearGrid> & refGrid = *(shared_ptr<XdmfCurvilinearGrid> *)(grid);
  shared_ptr<XdmfArray> & refDimensions = *(shared_ptr<XdmfArray> *)(dimensions);
  refGrid->setDimensions(refDimensions);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfCurvilinearGridSetGeometry(XDMFCURVILINEARGRID * grid, XDMFGEOMETRY * geometry, int passControl)
{
  shared_ptr<XdmfCurvilinearGrid> & refGrid = *(shared_ptr<XdmfCurvilinearGrid> *)(grid);
  shared_ptr<XdmfGeometry> & refGeometry = *(shared_ptr<XdmfGeometry> *)(geometry);
  refGrid->setGeometry(refGeometry);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfCurvilinearGrid, XDMFCURVILINEARGRID)
XDMF_GRID_C_CHILD_WRAPPER(XdmfCurvilinearGrid, XDMFCURVILINEARGRID)
