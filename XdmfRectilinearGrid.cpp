/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfRectilinearGrid.cpp                                             */
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
#include "XdmfRectilinearGrid.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfError.hpp"

/**
 * PIMPL
 */
class XdmfRectilinearGrid::XdmfRectilinearGridImpl {

public:

  class XdmfGeometryRectilinear : public XdmfGeometry
  {

  public:

    static shared_ptr<XdmfGeometryRectilinear>
    New(XdmfRectilinearGrid * const rectilinearGrid)
    {
      shared_ptr<XdmfGeometryRectilinear>
        p(new XdmfGeometryRectilinear(rectilinearGrid));
      return p;
    }

    unsigned int
    getNumberPoints() const
    {
      const shared_ptr<const XdmfArray> dimensions =
        mRectilinearGrid->getDimensions();
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
      const std::vector<shared_ptr<XdmfArray> > & coordinates =
        mRectilinearGrid->getCoordinates();
      for (unsigned int i = 0; i < coordinates.size(); ++i)
      {
        coordinates[i]->accept(visitor);
      }
    }

  private:

    XdmfGeometryRectilinear(XdmfRectilinearGrid * const rectilinearGrid) :
      mRectilinearGrid(rectilinearGrid)
    {
      this->setType(XdmfGeometryTypeRectilinear::New(mRectilinearGrid));
    }

    const XdmfRectilinearGrid * const mRectilinearGrid;
  };

  class XdmfGeometryTypeRectilinear : public XdmfGeometryType
  {

  public:

    static shared_ptr<const XdmfGeometryTypeRectilinear>
    New(const XdmfRectilinearGrid * const rectilinearGrid)
    {
      shared_ptr<const XdmfGeometryTypeRectilinear>
        p(new XdmfGeometryTypeRectilinear(rectilinearGrid));
      return p;
    }

    unsigned int
    getDimensions() const
    {
      return mRectilinearGrid->getDimensions()->getSize();
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      const unsigned int dimensions = this->getDimensions();
      if(dimensions == 3) {
        collectedProperties["Type"] = "VXVYVZ";
      }
      else if(dimensions == 2) {
        collectedProperties["Type"] = "VXVY";
      }
      else {
        collectedProperties["Type"] = "VECTORED";
      }
    }

  private:

    XdmfGeometryTypeRectilinear(const XdmfRectilinearGrid * const rectilinearGrid) :
      XdmfGeometryType("", 0),
      mRectilinearGrid(rectilinearGrid)
    {
    }

    const XdmfRectilinearGrid * const mRectilinearGrid;

  };

  class XdmfTopologyRectilinear : public XdmfTopology
  {

  public:

    static shared_ptr<XdmfTopologyRectilinear>
    New(const XdmfRectilinearGrid * const rectilinearGrid)
    {
      shared_ptr<XdmfTopologyRectilinear>
        p(new XdmfTopologyRectilinear(rectilinearGrid));
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
        mRectilinearGrid->getDimensions();
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

    XdmfTopologyRectilinear(const XdmfRectilinearGrid * const rectilinearGrid) :
      mRectilinearGrid(rectilinearGrid)
    {
      this->setType(XdmfTopologyTypeRectilinear::New(rectilinearGrid));
    }

    const XdmfRectilinearGrid * const mRectilinearGrid;
  };

  class XdmfTopologyTypeRectilinear : public XdmfTopologyType
  {

  public:

    static shared_ptr<const XdmfTopologyTypeRectilinear>
    New(const XdmfRectilinearGrid * const rectilinearGrid)
    {
      shared_ptr<const XdmfTopologyTypeRectilinear>
        p(new XdmfTopologyTypeRectilinear(rectilinearGrid));
      return p;
    }

    unsigned int
    getEdgesPerElement() const
    {
      return calculateHypercubeNumElements(mRectilinearGrid->getDimensions()->getSize(), 1);
    }

    unsigned int
    getFacesPerElement() const
    {
      return calculateHypercubeNumElements(mRectilinearGrid->getDimensions()->getSize(), 2);
    }

    unsigned int
    getNodesPerElement() const
    {
      return calculateHypercubeNumElements(mRectilinearGrid->getDimensions()->getSize(), 0);
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      shared_ptr<const XdmfArray> dimensions = 
        mRectilinearGrid->getDimensions();
      if(dimensions->getSize() == 3) {
        collectedProperties["Type"] = "3DRectMesh";
      }
      else if(dimensions->getSize() == 2) {
        collectedProperties["Type"] = "2DRectMesh";
      }
      else {
        collectedProperties["Type"] = "RectMesh";
      }
      collectedProperties["Dimensions"] = dimensions->getValuesString();
    }

  private:

    XdmfTopologyTypeRectilinear(const XdmfRectilinearGrid * const rectilinearGrid) :
      XdmfTopologyType(0,
                       0,
                       std::vector<shared_ptr<const XdmfTopologyType> >(),
                       0,
                       "foo",
                       XdmfTopologyType::Structured,
                       0x1101),
      mRectilinearGrid(rectilinearGrid)
    {
    }

    const XdmfRectilinearGrid * const mRectilinearGrid;

  };

  XdmfRectilinearGridImpl(const std::vector<shared_ptr<XdmfArray> > & coordinates) :
    mCoordinates(coordinates.begin(), coordinates.end())
  {
  }

  std::vector<shared_ptr<XdmfArray> > mCoordinates;

};

shared_ptr<XdmfRectilinearGrid>
XdmfRectilinearGrid::New(const shared_ptr<XdmfArray> xCoordinates,
                         const shared_ptr<XdmfArray> yCoordinates)
{
  std::vector<shared_ptr<XdmfArray> > axesCoordinates;
  axesCoordinates.resize(2);
  axesCoordinates[0] = xCoordinates;
  axesCoordinates[1] = yCoordinates;
  shared_ptr<XdmfRectilinearGrid> p(new XdmfRectilinearGrid(axesCoordinates));
  return p;
}

shared_ptr<XdmfRectilinearGrid>
XdmfRectilinearGrid::New(const shared_ptr<XdmfArray> xCoordinates,
                         const shared_ptr<XdmfArray> yCoordinates,
                         const shared_ptr<XdmfArray> zCoordinates)
{
  std::vector<shared_ptr<XdmfArray> > axesCoordinates;
  axesCoordinates.resize(3);
  axesCoordinates[0] = xCoordinates;
  axesCoordinates[1] = yCoordinates;
  axesCoordinates[2] = zCoordinates;
  shared_ptr<XdmfRectilinearGrid> p(new XdmfRectilinearGrid(axesCoordinates));
  return p;
}

shared_ptr<XdmfRectilinearGrid>
XdmfRectilinearGrid::New(const std::vector<shared_ptr<XdmfArray> > & axesCoordinates)
{
  shared_ptr<XdmfRectilinearGrid> p(new XdmfRectilinearGrid(axesCoordinates));
  return p;
}

XdmfRectilinearGrid::XdmfRectilinearGrid(const std::vector<shared_ptr<XdmfArray> > & axesCoordinates) :
  XdmfGrid(XdmfRectilinearGridImpl::XdmfGeometryRectilinear::New(this),
           XdmfRectilinearGridImpl::XdmfTopologyRectilinear::New(this))
{
  mImpl = new XdmfRectilinearGridImpl(axesCoordinates);
}

XdmfRectilinearGrid::~XdmfRectilinearGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfRectilinearGrid::ItemTag = "Grid";

void
XdmfRectilinearGrid::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  XdmfGrid::copyGrid(sourceGrid);
  if (shared_ptr<XdmfRectilinearGrid> classedGrid = shared_dynamic_cast<XdmfRectilinearGrid>(sourceGrid))
  {
    // Copy stucture from read grid to this grid
    this->setCoordinates(classedGrid->getCoordinates());
  }
}

shared_ptr<XdmfArray>
XdmfRectilinearGrid::getCoordinates(const unsigned int axisIndex)
{
  return const_pointer_cast<XdmfArray>(static_cast<const XdmfRectilinearGrid &>
				       (*this).getCoordinates(axisIndex));
}

shared_ptr<const XdmfArray>
XdmfRectilinearGrid::getCoordinates(const unsigned int axisIndex) const
{
  if(axisIndex < mImpl->mCoordinates.size()) {
    return mImpl->mCoordinates[axisIndex];
  }
  return shared_ptr<XdmfArray>();
}

std::vector<shared_ptr<XdmfArray> >
XdmfRectilinearGrid::getCoordinates()
{
  return static_cast<const XdmfRectilinearGrid &>(*this).getCoordinates();
}

const std::vector<shared_ptr<XdmfArray> >
XdmfRectilinearGrid::getCoordinates() const
{
  return mImpl->mCoordinates;
}

shared_ptr<XdmfArray>
XdmfRectilinearGrid::getDimensions()
{
  return const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRectilinearGrid &>(*this).getDimensions());
}

shared_ptr<const XdmfArray>
XdmfRectilinearGrid::getDimensions() const
{
  shared_ptr<XdmfArray> dimensions = XdmfArray::New();
  std::vector<shared_ptr<XdmfArray> > heldCoordinates =
    ((XdmfRectilinearGridImpl*)mImpl)->mCoordinates;
  dimensions->reserve(static_cast<unsigned int>(heldCoordinates.size()));
  for (unsigned int i = 0; i < heldCoordinates.size(); ++i)
  {
    dimensions->pushBack(heldCoordinates[i]->getSize());
  }
  return dimensions;
}

void
XdmfRectilinearGrid::populateItem(const std::map<std::string, std::string> & itemProperties,
                                  const std::vector<shared_ptr<XdmfItem> > & childItems,
                                  const XdmfCoreReader * const reader)
{
  XdmfGrid::populateItem(itemProperties, childItems, reader);

  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfRectilinearGrid> rectilinearGrid =
       shared_dynamic_cast<XdmfRectilinearGrid>(*iter)) {
      if(rectilinearGrid->getGeometry()->getType()->getDimensions() > 0) {
        this->setCoordinates(rectilinearGrid->getCoordinates());
        break;
      }
    }
  }
}

void
XdmfRectilinearGrid::read()
{
  if (mGridController)
  {
    if (shared_ptr<XdmfRectilinearGrid> grid = shared_dynamic_cast<XdmfRectilinearGrid>(mGridController->read()))
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
XdmfRectilinearGrid::release()
{
  XdmfGrid::release();
  ((XdmfRectilinearGridImpl*)mImpl)->mCoordinates.clear();
}

void
XdmfRectilinearGrid::setCoordinates(const unsigned int axisIndex,
                                    const shared_ptr<XdmfArray> axisCoordinates)
{
  if(mImpl->mCoordinates.size() <= axisIndex) {
    mImpl->mCoordinates.reserve(axisIndex + 1);
    unsigned int numArraysToInsert =
      axisIndex - static_cast<unsigned int>(mImpl->mCoordinates.size()) + 1;
    for(unsigned int i=0; i<numArraysToInsert; ++i) {
      mImpl->mCoordinates.push_back(XdmfArray::New());
    }
  }
  mImpl->mCoordinates[axisIndex] = axisCoordinates;
  this->setIsChanged(true);
}

void
XdmfRectilinearGrid::setCoordinates(const std::vector<shared_ptr<XdmfArray> > axesCoordinates)
{
  mImpl->mCoordinates = axesCoordinates;
  this->setIsChanged(true);
}

// C Wrappers

XDMFRECTILINEARGRID * XdmfRectilinearGridNew(XDMFARRAY ** axesCoordinates, unsigned int numCoordinates, int passControl)
{
  std::vector<shared_ptr<XdmfArray> > holderVector;
  for (unsigned int i = 0; i < numCoordinates; ++i) {
    shared_ptr<XdmfArray> & refArray = *(shared_ptr<XdmfArray> *)(axesCoordinates[i]);
    holderVector.push_back(refArray);
  }
  shared_ptr<XdmfRectilinearGrid> * generatedGrid = 
    new shared_ptr<XdmfRectilinearGrid>(XdmfRectilinearGrid::New(holderVector));
  return (XDMFRECTILINEARGRID *) generatedGrid;
}

XDMFRECTILINEARGRID * XdmfRectilinearGridNew2D(XDMFARRAY * xCoordinates, XDMFARRAY * yCoordinates, int passControl)
{
   shared_ptr<XdmfArray> & refXCoordinates = *(shared_ptr<XdmfArray> *)(xCoordinates);
   shared_ptr<XdmfArray> & refYCoordinates = *(shared_ptr<XdmfArray> *)(yCoordinates);
   shared_ptr<XdmfRectilinearGrid> * generatedGrid = 
     new shared_ptr<XdmfRectilinearGrid>(XdmfRectilinearGrid::New(refXCoordinates,
								  refYCoordinates));
   return (XDMFRECTILINEARGRID *) generatedGrid;
}

XDMFRECTILINEARGRID * XdmfRectilinearGridNew3D(XDMFARRAY * xCoordinates, XDMFARRAY * yCoordinates, XDMFARRAY * zCoordinates, int passControl)
{
   shared_ptr<XdmfArray> & refXCoordinates = *(shared_ptr<XdmfArray> *)(xCoordinates);
   shared_ptr<XdmfArray> & refYCoordinates = *(shared_ptr<XdmfArray> *)(yCoordinates);
   shared_ptr<XdmfArray> & refZCoordinates = *(shared_ptr<XdmfArray> *)(zCoordinates);
   shared_ptr<XdmfRectilinearGrid> * generatedGrid = 
     new shared_ptr<XdmfRectilinearGrid>(XdmfRectilinearGrid::New(refXCoordinates,
								  refYCoordinates,
								  refZCoordinates));
   return (XDMFRECTILINEARGRID *) generatedGrid;
}

XDMFARRAY * XdmfRectilinearGridGetCoordinatesByIndex(XDMFRECTILINEARGRID * grid, unsigned int axisIndex, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  shared_ptr<XdmfArray> * p = new shared_ptr<XdmfArray>(refGrid->getCoordinates(axisIndex));
  return (XDMFARRAY *) p;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFARRAY ** XdmfRectilinearGridGetCoordinates(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  XDMFARRAY ** returnPointer;
  std::vector<shared_ptr<XdmfArray> > heldCoordinates = refGrid->getCoordinates();
  returnPointer = (XDMFARRAY **)malloc(sizeof(XDMFARRAY*) * (heldCoordinates.size() + 1));
  for (unsigned int i = 0; i < heldCoordinates.size(); ++i) {
    shared_ptr<XdmfArray> * p = new shared_ptr<XdmfArray>(heldCoordinates[i]);
    returnPointer[i] = (XDMFARRAY*) p;
  }
  returnPointer[heldCoordinates.size()] = NULL;
  return returnPointer;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

int XdmfRectilinearGridGetNumberCoordinates(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  std::vector<shared_ptr<XdmfArray> > heldCoordinates = refGrid->getCoordinates();
  return static_cast<int>(heldCoordinates.size());
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

XDMFARRAY * XdmfRectilinearGridGetDimensions(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  shared_ptr<XdmfArray> * p = new shared_ptr<XdmfArray>(refGrid->getDimensions());
  return (XDMFARRAY *) p;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void XdmfRectilinearGridSetCoordinates(XDMFRECTILINEARGRID * grid, XDMFARRAY ** axesCoordinates, unsigned int numCoordinates, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  std::vector<shared_ptr<XdmfArray> > holderVector;
  for (unsigned int i = 0; i < numCoordinates; ++i) {
    shared_ptr<XdmfArray> & refArray = *(shared_ptr<XdmfArray> *)(axesCoordinates[i]);
    holderVector.push_back(refArray);
  }
  refGrid->setCoordinates(holderVector);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfRectilinearGridSetCoordinatesByIndex(XDMFRECTILINEARGRID * grid, unsigned int index, XDMFARRAY * coordinates, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfRectilinearGrid> & refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(grid);
  shared_ptr<XdmfArray> & refCoordinates = *(shared_ptr<XdmfArray> *)(coordinates);
  refGrid->setCoordinates(index, refCoordinates);
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfRectilinearGrid, XDMFRECTILINEARGRID)
XDMF_GRID_C_CHILD_WRAPPER(XdmfRectilinearGrid, XDMFRECTILINEARGRID)
