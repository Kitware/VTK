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
class XdmfRectilinearGrid::XdmfRectilinearGridImpl : public XdmfGridImpl {

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
    mGridType = "Rectilinear";
  }

  XdmfGridImpl * duplicate()
  {
    return new XdmfRectilinearGridImpl(mCoordinates);
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

XdmfRectilinearGrid::XdmfRectilinearGrid(XdmfRectilinearGrid & refGrid):
  XdmfGrid(refGrid)
{
  mTopology = XdmfRectilinearGridImpl::XdmfTopologyRectilinear::New(this);
  mGeometry = XdmfRectilinearGridImpl::XdmfGeometryRectilinear::New(this);
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
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRectilinearGrid &>
     (*this).getCoordinates(axisIndex));
}

shared_ptr<const XdmfArray>
XdmfRectilinearGrid::getCoordinates(const unsigned int axisIndex) const
{
  if(axisIndex < ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates.size()) {
    return ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates[axisIndex];
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
  return ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates;
}

shared_ptr<XdmfArray>
XdmfRectilinearGrid::getDimensions()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfRectilinearGrid &>(*this).getDimensions());
}

shared_ptr<const XdmfArray>
XdmfRectilinearGrid::getDimensions() const
{
  shared_ptr<XdmfArray> dimensions = XdmfArray::New();
  std::vector<shared_ptr<XdmfArray> > heldCoordinates =
    ((XdmfRectilinearGridImpl*)mImpl)->mCoordinates;
  dimensions->reserve(heldCoordinates.size());
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
    else if (shared_dynamic_cast<XdmfGrid>(mGridController->read()))
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
  if(((XdmfRectilinearGridImpl *)mImpl)->mCoordinates.size() <= axisIndex) {
    ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates.reserve(axisIndex + 1);
    unsigned int numArraysToInsert =
      axisIndex - ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates.size() + 1;
    for(unsigned int i=0; i<numArraysToInsert; ++i) {
      ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates.push_back(XdmfArray::New());
    }
  }
  ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates[axisIndex] = axisCoordinates;
  this->setIsChanged(true);
}

void
XdmfRectilinearGrid::setCoordinates(const std::vector<shared_ptr<XdmfArray> > axesCoordinates)
{
  ((XdmfRectilinearGridImpl *)mImpl)->mCoordinates = axesCoordinates;
  this->setIsChanged(true);
}

// C Wrappers

XDMFRECTILINEARGRID * XdmfRectilinearGridNew(XDMFARRAY ** axesCoordinates, unsigned int numCoordinates, int passControl)
{
  try
  {
    std::vector<shared_ptr<XdmfArray> > holderVector;
    for (unsigned int i = 0; i < numCoordinates; ++i) {
      if (passControl) {
        holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i]));
      }
      else {
        holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i], XdmfNullDeleter()));
      }
    }
    shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(holderVector);
    return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
  }
  catch (...)
  {
    std::vector<shared_ptr<XdmfArray> > holderVector;
    for (unsigned int i = 0; i < numCoordinates; ++i) {
      if (passControl) {
        holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i]));
      }
      else {
        holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i], XdmfNullDeleter()));
      }
    }
    shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(holderVector);
    return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
  }
}

XDMFRECTILINEARGRID * XdmfRectilinearGridNew2D(XDMFARRAY * xCoordinates, XDMFARRAY * yCoordinates, int passControl)
{
  try
  {
    if (passControl) {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates, XdmfNullDeleter()));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
  }
  catch (...)
  {
    if (passControl) {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates, XdmfNullDeleter()));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
  }
}

XDMFRECTILINEARGRID * XdmfRectilinearGridNew3D(XDMFARRAY * xCoordinates, XDMFARRAY * yCoordinates, XDMFARRAY * zCoordinates, int passControl)
{
  try
  {
    if (passControl) {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)zCoordinates));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)zCoordinates, XdmfNullDeleter()));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
  }
  catch (...)
  {
    if (passControl) {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)zCoordinates));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
    else {
      shared_ptr<XdmfRectilinearGrid> generatedGrid = XdmfRectilinearGrid::New(shared_ptr<XdmfArray>((XdmfArray *)xCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)yCoordinates, XdmfNullDeleter()),
                                                                               shared_ptr<XdmfArray>((XdmfArray *)zCoordinates, XdmfNullDeleter()));
      return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*generatedGrid.get()))));
    }
  }
}

XDMFARRAY * XdmfRectilinearGridGetCoordinatesByIndex(XDMFRECTILINEARGRID * grid, unsigned int axisIndex, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
  return (XDMFARRAY *)((void *)(gridPointer->getCoordinates(axisIndex).get()));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFARRAY ** XdmfRectilinearGridGetCoordinates(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    XDMFARRAY ** returnPointer;
    XdmfItem * classedPointer = (XdmfItem *)grid;
    XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
    std::vector<shared_ptr<XdmfArray> > heldCoordinates = gridPointer->getCoordinates();
    returnPointer = new XDMFARRAY *[heldCoordinates.size()]();
    for (unsigned int i = 0; i < heldCoordinates.size(); ++i) {
      XDMFARRAY * insertArray = (XDMFARRAY *)((void *)(new XdmfArray(*(heldCoordinates[i].get()))));
      returnPointer[i] = insertArray;
    }
    return returnPointer;
  }
  catch (...)
  {
    XDMFARRAY ** returnPointer;
    XdmfItem * classedPointer = (XdmfItem *)grid;
    XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
    std::vector<shared_ptr<XdmfArray> > heldCoordinates = gridPointer->getCoordinates();
    returnPointer = new XDMFARRAY *[heldCoordinates.size()]();
    for (unsigned int i = 0; i < heldCoordinates.size(); ++i) {
      XDMFARRAY * insertArray = (XDMFARRAY *)((void *)(new XdmfArray(*(heldCoordinates[i].get()))));
      returnPointer[i] = insertArray;
    }
    return returnPointer;
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

int XdmfRectilinearGridGetNumberCoordinates(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
  std::vector<shared_ptr<XdmfArray> > heldCoordinates = gridPointer->getCoordinates();
  return heldCoordinates.size();
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

XDMFARRAY * XdmfRectilinearGridGetDimensions(XDMFRECTILINEARGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    XdmfArray * copyArray;
    shared_ptr<XdmfArray> returnDimensions;
    XdmfItem * classedPointer = (XdmfItem *)grid;
    XdmfGrid * classedGrid = dynamic_cast<XdmfGrid *>(classedPointer);
    XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedGrid);
    XDMFARRAY * returnArray;
    returnDimensions = gridPointer->getDimensions();
    copyArray = new XdmfArray(*(returnDimensions.get()));
    void * copyVoid = (void *)copyArray;
    returnArray = (XDMFARRAY *) copyVoid;
    returnDimensions.reset();
    return returnArray;
  }
  catch (...)
  {
    XdmfArray * copyArray;
    shared_ptr<XdmfArray> returnDimensions;
    XdmfItem * classedPointer = (XdmfItem *)grid;
    XdmfGrid * classedGrid = dynamic_cast<XdmfGrid *>(classedPointer);
    XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedGrid);
    XDMFARRAY * returnArray;
    returnDimensions = gridPointer->getDimensions();
    copyArray = new XdmfArray(*(returnDimensions.get()));
    void * copyVoid = (void *)copyArray;
    returnArray = (XDMFARRAY *) copyVoid;
    returnDimensions.reset();
    return returnArray;
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void XdmfRectilinearGridSetCoordinates(XDMFRECTILINEARGRID * grid, XDMFARRAY ** axesCoordinates, unsigned int numCoordinates, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
  std::vector<shared_ptr<XdmfArray> > holderVector;
  for (unsigned int i = 0; i < numCoordinates; ++i) {
    if (passControl) {
      holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i]));
    }
    else {
      holderVector.push_back(shared_ptr<XdmfArray>((XdmfArray *)axesCoordinates[i], XdmfNullDeleter()));
    }
  }
  gridPointer->setCoordinates(holderVector);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfRectilinearGridSetCoordinatesByIndex(XDMFRECTILINEARGRID * grid, unsigned int index, XDMFARRAY * coordinates, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfRectilinearGrid * gridPointer = dynamic_cast<XdmfRectilinearGrid *>(classedPointer);
  if (passControl) {
    gridPointer->setCoordinates(index, shared_ptr<XdmfArray>((XdmfArray *)coordinates));
  }
  else {
    gridPointer->setCoordinates(index, shared_ptr<XdmfArray>((XdmfArray *)coordinates, XdmfNullDeleter()));
  }
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfRectilinearGrid, XDMFRECTILINEARGRID)
XDMF_GRID_C_CHILD_WRAPPER(XdmfRectilinearGrid, XDMFRECTILINEARGRID)
