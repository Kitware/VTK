/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter                                              */
/*     kenneth.leiter@arl.army.mil                                 */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2009 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include <Xdmf.h>
#include <XdmfSet.h>

#include <stdio.h>
#include <sstream>
#include <map>
#include <stack>
#include <vector>

#include "XdmfFortran.h"

// This works with g77. Different compilers require different
// name mangling.
#if !defined(WIN32)
#define XdmfInit xdmfinit_
#define XdmfSetTime xdmfsettime_
#define XdmfAddCollection xdmfaddcollection_
#define XdmfCloseCollection xdmfclosecollection_
#define XdmfSetGridTopology xdmfsetgridtopology_
#define XdmfSetGridTopologyFromShape xdmfsetgridtopologyfromshape_
#define XdmfSetGridGeometry xdmfsetgridgeometry_
#define XdmfAddGridAttribute xdmfaddgridattribute_
#define XdmfAddGridAttributeFromShape xdmfaddgridattributefromshape_
#define XdmfAddGridInformation xdmfaddgridinformation_
#define XdmfAddCollectionAttribute xdmfaddcollectionattribute_
#define XdmfAddCollectionInformation xdmfaddcollectioninformation_
#define XdmfAddArray xdmfaddarray_
#define XdmfReadFile xdmfreadfile_
#define XdmfReadGrid xdmfreadgrid_
#define XdmfReadGridAtIndex xdmfreadgridatindex_
#define XdmfGetNumberOfGrids xdmfgetnumberofgrids_
#define XdmfGetNumberOfPoints xdmfgetnumberofpoints_
#define XdmfReadPointValues xdmfreadpointvalues_
#define XdmfGetNumberOfAttributeValues xdmfgetnumberofattributevalues_
#define XdmfReadAttributeValues xdmfreadattributevalues_
#define XdmfReadInformationValue xdmfreadinformationvalue_
#define XdmfGetTime xdmfgettime_
#define XdmfWriteGrid xdmfwritegrid_
#define XdmfWriteToFile xdmfwritetofile_
#define XdmfSerialize xdmfserialize_
#define XdmfGetDOM xdmfgetdom_
#define XdmfClose xdmfclose_
#endif
/**
 *
 * Initialize a new Xdmf file.
 *
 */
XdmfFortran::XdmfFortran(char * outputName)
{
  myDOM = new XdmfDOM();
  myRoot = new XdmfRoot();
  myDomain = new XdmfDomain();
  myTopology = NULL;
  myGeometry = NULL;
  currentTime = -1;

  myRoot->SetDOM(myDOM);
  myRoot->Build();
  myRoot->Insert(myDomain);
  myName = outputName;
}

XdmfFortran::~XdmfFortran()
{
  this->Destroy();
}

void
XdmfFortran::Destroy()
{
  currentTime = -1;

  delete myGeometry;
  myGeometry = NULL;
  delete myTopology;
  myTopology = NULL;

  while (!myAttributes.empty())
  {
    delete myAttributes.back();
    myAttributes.pop_back();
  }

  while (!myInformations.empty())
  {
    delete myInformations.back();
    myInformations.pop_back();
  }

  while (!myCollections.empty())
  {
    delete myCollections.top();
    myCollections.pop();
  }

  delete myDOM;
  delete myRoot;
  delete myDomain;
}

/**
 *
 * Set a time to be assigned to the next grid.
 *
 */
void
XdmfFortran::SetTime(double * t)
{
  currentTime = *t;
}

/**
 *
 * Add a collection to the XdmfDOM.  Collections can be 'Spatial' or 'Temporal' type.
 * Nested collections are supported.
 *
 */
void
XdmfFortran::AddCollection(char * collectionType)
{
  XdmfGrid * currentCollection = new XdmfGrid();
  currentCollection->SetGridType(XDMF_GRID_COLLECTION);
  currentCollection->SetCollectionTypeFromString(collectionType);
  if (myCollections.empty())
  {
    myDomain->Insert(currentCollection);
  }
  else
  {
    myCollections.top()->Insert(currentCollection);
  }
  currentCollection->Build();
  myCollections.push(currentCollection);
}

/**
 *
 * Close the current open collection.  If within a nested collection, close
 * the most deeply nested collection.
 *
 */
void
XdmfFortran::CloseCollection()
{
  if (!myCollections.empty())
  {
    delete myCollections.top();
    myCollections.pop();
  }
}

/**
 *
 * Set the topology type to be assigned to the next grid.
 * Only XDMF_INT_32 type currently supported for Topology --> INTEGER*4
 *
 */
void
XdmfFortran::SetGridTopology(char * topologyType, int * numberOfElements, XdmfInt32 * conns)
{
  std::stringstream shape;
  shape << *numberOfElements;
  this->SetGridTopologyFromShape(topologyType, (char*) shape.str().c_str(), conns);
}

/**
 *
 * Set the topology type to be assigned to the next grid.
 * Only XDMF_INT_32 type currently supported for Topology --> INTEGER*4
 *
 */
void
XdmfFortran::SetGridTopologyFromShape(char * topologyType, char * shape, XdmfInt32 * conns)
{
  myTopology = new XdmfTopology();
  myTopology->SetTopologyTypeFromString(topologyType);
  myTopology->GetShapeDesc()->SetShapeFromString(shape);

  if (myTopology->GetClass() != XDMF_STRUCTURED && myTopology->GetTopologyType() != XDMF_POLYVERTEX)
  {

    XdmfArray * myConnections = myTopology->GetConnectivity();
    myConnections->SetNumberType(XDMF_INT32_TYPE);
    myConnections->SetNumberOfElements(myTopology->GetNumberOfElements()
        * myTopology->GetNodesPerElement());
    myConnections->SetValues(0, conns, myTopology->GetNumberOfElements()
        * myTopology->GetNodesPerElement());
  }
}

/**
 *
 * Set the geometry type to be assigned to the next grid.
 *
 */
void
XdmfFortran::SetGridGeometry(char * geometryType, char * numberType, int * numberOfPoints,
    XdmfPointer * points)
{
  myGeometry = new XdmfGeometry();
  myGeometry->SetGeometryTypeFromString(geometryType);
  myGeometry->SetNumberOfPoints(*numberOfPoints);

  XdmfArray * myPoints = myGeometry->GetPoints();
  myPoints->SetNumberTypeFromString(numberType);

  switch (myGeometry->GetGeometryType())
    {
  case XDMF_GEOMETRY_XYZ:
    myPoints->SetNumberOfElements(*numberOfPoints * 3);
    break;
  case XDMF_GEOMETRY_X_Y_Z:
    myPoints->SetNumberOfElements(*numberOfPoints * 3);
    break;
  case XDMF_GEOMETRY_XY:
    myPoints->SetNumberOfElements(*numberOfPoints * 2);
    break;
  case XDMF_GEOMETRY_X_Y:
    myPoints->SetNumberOfElements(*numberOfPoints * 2);
    break;
  case XDMF_GEOMETRY_VXVYVZ:
    //TODO: FIX THIS
    myPoints->SetNumberOfElements(*numberOfPoints * 3);
    break;
  case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
    myPoints->SetNumberOfElements(6);
    break;
  default:
    myPoints->SetNumberOfElements(*numberOfPoints * 3);
    break;
    }
  WriteToXdmfArray(myPoints, points);
}

/**
 *
 * Add an attribute to be written to the next grid.  Multiple attributes can
 * be added and written to a single grid.
 *
 */
void
XdmfFortran::AddGridAttribute(char * attributeName, char * numberType, char * attributeCenter,
    char * attributeType, int * numberOfPoints, XdmfPointer * data)
{
  XdmfAttribute * currAttribute = new XdmfAttribute();
  currAttribute->SetName(attributeName);
  currAttribute->SetAttributeCenterFromString(attributeCenter);
  currAttribute->SetAttributeTypeFromString(attributeType);
  currAttribute->SetDeleteOnGridDelete(true);

  XdmfArray * array = currAttribute->GetValues();
  array->SetNumberTypeFromString(numberType);
  array->SetNumberOfElements(*numberOfPoints);
  WriteToXdmfArray(array, data);
  myAttributes.push_back(currAttribute);
}

/**
 *
 * Add an attribute with shape to be written to the next grid.  Multiple attributes can
 * be added and written to a single grid. Dimensions are specified using shape string rather than int.
 * Extra argument supports setting units string of attribute.
 *
 */
void
XdmfFortran::AddGridAttributeFromShape(char * attributeName, char * numberType,
    char * attributeCenter, char * attributeType, char * shape, char * units, XdmfPointer * data)
{
  XdmfAttribute * currAttribute = new XdmfAttribute();
  currAttribute->SetName(attributeName);
  currAttribute->SetUnits(units);
  currAttribute->SetAttributeCenterFromString(attributeCenter);
  currAttribute->SetAttributeTypeFromString(attributeType);
  currAttribute->SetDeleteOnGridDelete(true);

  XdmfArray * array = currAttribute->GetValues();
  array->SetNumberTypeFromString(numberType);
  array->SetShapeFromString(shape); // dims via shape string
  WriteToXdmfArray(array, data);
  myAttributes.push_back(currAttribute);
}

/**
 *
 * Add an attribute to be written to the next grid.  Multiple attributes can
 * be added and written to a single grid.
 *
 */
void
XdmfFortran::AddGridInformation(char * informationName, char * value)
{
  XdmfInformation * currInfo = new XdmfInformation();
  currInfo->SetName(informationName);
  currInfo->SetValue(value);
  currInfo->SetDeleteOnGridDelete(true);

  myInformations.push_back(currInfo);
}

/**
 *
 * Add an attribute to the current collection.  If we are not within a collection do nothing!
 *
 */
void
XdmfFortran::AddCollectionAttribute(char * attributeName, char * numberType,
    char * attributeCenter, char * attributeType, int * numberOfPoints, XdmfPointer * data)
{
  if (!myCollections.empty())
  {
    XdmfAttribute * currAttribute = new XdmfAttribute();
    currAttribute->SetName(attributeName);
    currAttribute->SetAttributeCenterFromString(attributeCenter);
    currAttribute->SetAttributeTypeFromString(attributeType);
    currAttribute->SetDeleteOnGridDelete(true);

    XdmfArray * array = currAttribute->GetValues();
    array->SetNumberTypeFromString(numberType);
    array->SetNumberOfElements(*numberOfPoints);
    WriteToXdmfArray(array, data);
    myCollections.top()->Insert(currAttribute);
    myCollections.top()->Build();
  }
}

/**
 *
 * Add an attribute to the current collection.  If we are not within a collection add to the top level domain
 *
 */
void
XdmfFortran::AddCollectionInformation(char * informationName, char * value)
{
  XdmfInformation * currInfo = new XdmfInformation();
  currInfo->SetName(informationName);
  currInfo->SetValue(value);
  currInfo->SetDeleteOnGridDelete(true);
  if (!myCollections.empty())
  {

    myCollections.top()->Insert(currInfo);
    myCollections.top()->Build();
  }
  else
  {
    myDomain->Insert(currInfo);
    myDomain->Build();
  }
}

/**
 *
 * Write out "generic" data to XDMF.  This writes out data to the end of the top-level domain or the current collection.  It is independent of any grids.
 * Currently supports only writing a single dataitem.
 *
 */
void
XdmfFortran::AddArray(char * name, char * numberType, int * numberOfValues, XdmfPointer * data)
{
  XdmfSet * currSet = new XdmfSet();
  currSet->SetDOM(myDOM);
  currSet->SetSetType(XDMF_SET_TYPE_NODE);
  currSet->SetName(name);
  currSet->SetDeleteOnGridDelete(true);

  // Copy Elements from Set to XdmfArray
  XdmfArray * array = currSet->GetIds();
  array->SetNumberTypeFromString(numberType);
  array->SetNumberOfElements(*numberOfValues);
  std::stringstream heavyDataName;
  heavyDataName << myName << ".h5:/" << name;
  ;
  array->SetHeavyDataSetName(heavyDataName.str().c_str());
  WriteToXdmfArray(array, data);

  if (!myCollections.empty())
  {
    myCollections.top()->Insert(currSet);
    currSet->Build();
  }
  else
  {
    XdmfGrid * myGrid = new XdmfGrid();
    myGrid->SetDOM(myDOM);
    myGrid->SetElement(myDOM->FindElement("Domain"));
    myGrid->Insert(currSet);
    currSet->Build();
    delete myGrid;
  }
}

/**
 *
 * Read an Xdmf file into the current XdmfDOM.  Must call XdmfReadGrid() to read in associated geometry
 * topology, and attributes.
 *
 */
void
XdmfFortran::ReadFile(char * filePath)
{
  // Clear state and start over before reading file
  this->Destroy();

  myDOM = new XdmfDOM();
  myRoot = new XdmfRoot();
  myDomain = new XdmfDomain();

  myDOM->Parse(filePath);
  myDomain->SetElement(myDOM->FindElement("Domain"));
  myRoot->SetElement(myDOM->GetRoot());

  // Perhaps we should support collections more on this part?
  while (!myCollections.empty())
  {
    delete myCollections.top();
    myCollections.pop();
  }
  myGridPaths.clear();
  myGridNames.clear();
  this->ReadFilePriv(myDomain->GetElement());
}

void
XdmfFortran::ReadFilePriv(XdmfXmlNode currElement)
{
  XdmfGrid currGrid = XdmfGrid();
  for (int i = 0; i < myDOM->FindNumberOfElements("Grid", currElement); i++)
  {
    currGrid.SetDOM(myDOM);
    currGrid.SetElement(myDOM->FindElement("Grid", i, currElement));
    currGrid.Update();
    if (currGrid.GetGridType() != XDMF_GRID_COLLECTION)
    {
      myGridPaths.push_back(myDOM->GetPath(currGrid.GetElement()));
      std::string gridName = currGrid.GetName();
      if (gridName.find_last_of("_") != std::string::npos)
      {
        try
        {
          atoi(gridName.substr(gridName.find_last_of("_") + 1, gridName.size()
              - gridName.find_last_of("_")).c_str());
          gridName = gridName.substr(0, gridName.find_last_of("_"));
        }
        catch (int)
        {
        }
      }
      if (myGridNames.find(gridName.c_str()) == myGridNames.end())
      {
        myGridNames[gridName.c_str()] = 1;
      }
      else
      {
        myGridNames[gridName.c_str()]++;
      }
    }
    this->ReadFilePriv(currGrid.GetElement());
  }
}

/**
 *
 * Read a grid in the current XdmfDOM into XdmfGeometry, XdmfTopology, and XdmfAttribute elements.
 * An XdmfReadGrid() followed by a XdmfWriteGrid() will make a copy of the grid.
 *
 */
void
XdmfFortran::ReadGrid(char * gridName)
{
  ReadGridPriv(gridName, myDomain->GetElement());
}

/**
 *
 * Read a grid in the current XdmfDOM into XdmfGeometry, XdmfTopology, and XdmfAttribute elements.
 * An XdmfReadGrid() followed by a XdmfWriteGrid() will make a copy of the grid.
 *
 */
void
XdmfFortran::ReadGridAtIndex(int * gridIndex)
{
  ReadGridPriv(myGridPaths[*gridIndex].c_str());
}

/**
 *
 * Helper function for XdmfReadGrid.  Ensures that all grids are traversed and that the method works
 * even within collections.
 *
 */
void
XdmfFortran::ReadGridPriv(char * gridName, XdmfXmlNode currElement)
{
  XdmfGrid currGrid = XdmfGrid();
  for (int i = 0; i < myDOM->FindNumberOfElements("Grid", currElement); i++)
  {
    currGrid.SetDOM(myDOM);
    currGrid.SetElement(myDOM->FindElement("Grid", i, currElement));
    currGrid.Update();
    if (currGrid.GetGridType() != XDMF_GRID_COLLECTION)
    {
      if (strcmp(gridName, currGrid.GetName()) == 0)
      {
        return this->ReadGridPriv(myDOM->GetPath(currGrid.GetElement()));
      }
    }
    this->ReadGridPriv(gridName, currGrid.GetElement());
  }
}

/**
 *
 * Helper function for XdmfReadGrid.  Ensures that all grids are traversed and that the method works
 * even within collections.
 *
 */
void
XdmfFortran::ReadGridPriv(XdmfConstString gridPath)
{
  XdmfGrid currGrid = XdmfGrid();
  currGrid.SetDOM(myDOM);
  currGrid.SetElement(myDOM->FindElementByPath(gridPath));
  currGrid.Update();

  delete myGeometry;
  delete myTopology;

  myGeometry = new XdmfGeometry();
  myGeometry->SetGeometryType(currGrid.GetGeometry()->GetGeometryType());
  myGeometry->SetNumberOfPoints(currGrid.GetGeometry()->GetNumberOfPoints());
  myGeometry->SetPoints(currGrid.GetGeometry()->GetPoints()->Clone());

  myTopology = new XdmfTopology();
  myTopology->SetTopologyType(currGrid.GetTopology()->GetTopologyType());
  myTopology->SetNumberOfElements(currGrid.GetTopology()->GetNumberOfElements());
  myTopology->SetConnectivity(currGrid.GetTopology()->GetConnectivity()->Clone());

  while (!myAttributes.empty())
  {
    delete myAttributes.back();
    myAttributes.pop_back();
  }

  for (int j = 0; j < currGrid.GetNumberOfAttributes(); j++)
  {
    currGrid.GetAttribute(j)->Update();
    XdmfAttribute * currAttribute = new XdmfAttribute();
    currAttribute->SetName(currGrid.GetAttribute(j)->GetName());
    currAttribute->SetAttributeCenter(currGrid.GetAttribute(j)->GetAttributeCenter());
    currAttribute->SetAttributeType(currGrid.GetAttribute(j)->GetAttributeType());
    currAttribute->SetDeleteOnGridDelete(true);
    currAttribute->SetValues(currGrid.GetAttribute(j)->GetValues()->Clone());
    myAttributes.push_back(currAttribute);
  }

  for (int j = 0; j < currGrid.GetNumberOfInformations(); j++)
  {
    currGrid.GetInformation(j)->UpdateInformation();
    XdmfInformation * currInformation = new XdmfInformation();
    currInformation->SetName(currGrid.GetInformation(j)->GetName());
    currInformation->SetValue(currGrid.GetInformation(j)->GetValue());
    currInformation->SetDeleteOnGridDelete(true);
    myInformations.push_back(currInformation);
  }
}

/**
 *
 * Returns the number of grids in the current open file.  This ignores collections.
 *
 */
void
XdmfFortran::GetNumberOfGrids(XdmfInt32 * toReturn)
{
  *toReturn = myGridPaths.size();
}

/**
 *
 * Returns the number of points in the current open grid (the current active XdmfGeometry Element).  This is either
 * from a current read-in file or from a created but unwritten grid.  If no geometry element is present, return -1.
 *
 */
void
XdmfFortran::GetNumberOfPoints(XdmfInt32 * toReturn)
{
  if (myGeometry != NULL)
  {
    *toReturn = (XdmfInt32) myGeometry->GetNumberOfPoints();
  }
  else
  {
    *toReturn = -1;
  }
}

/**
 *
 * Reads the point values from the current geometry into the passed array pointer.  If the geometry has not been created
 * no values are read.
 *
 */
void
XdmfFortran::ReadPointValues(char * numberType, XdmfInt32 * startIndex, XdmfPointer * arrayToFill,
    XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride, XdmfInt32 * valuesStride)
{
  if (myGeometry != NULL)
  {
    this->ReadFromXdmfArray(myGeometry->GetPoints(), numberType, startIndex, arrayToFill,
        numberOfValues, arrayStride, valuesStride);
  }
}

/**
 *
 * Returns the number of values in the specified attribute.  Iterates over all current open attributes to find the
 * specified attribute name and returns the number of values it contains.  If no attribute is found, return -1.
 *
 */
void
XdmfFortran::GetNumberOfAttributeValues(char * attributeName, XdmfInt32 * toReturn)
{
  for (unsigned int i = 0; i < myAttributes.size(); i++)
  {
    if (strcmp(myAttributes[i]->GetName(), attributeName) == 0)
    {
      *toReturn = (XdmfInt32) myAttributes[i]->GetValues()->GetNumberOfElements();
      return;
    }
  }
  *toReturn = -1;
}

/**
 *
 * Reads the values from the specified attribute into the passed array pointer.  If the attribute cannot be found,
 * no values are read.
 *
 */
void
XdmfFortran::ReadAttributeValues(char * attributeName, char * numberType, XdmfInt32 * startIndex,
    XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride,
    XdmfInt32 * valuesStride)
{
  for (unsigned int i = 0; i < myAttributes.size(); i++)
  {
    if (strcmp(myAttributes[i]->GetName(), attributeName) == 0)
    {
      this->ReadFromXdmfArray(myAttributes[i]->GetValues(), numberType, startIndex, arrayToFill,
          numberOfValues, arrayStride, valuesStride);
    }
  }
}

/**
 *
 * Reads the values from the specified information element into the passed const string pointer.  If the information element cannot be found,
 * no values are passed.  Information elements at the top level domain are searched first, followed by the currently loaded grid.
 *
 */
void
XdmfFortran::ReadInformationValue(char * informationName, char * toReturn)
{
  // TODO: Make this work better for collections as well!
  for (unsigned int i = 0; i < myInformations.size(); i++)
  {
    if (strcmp(informationName, myInformations[i]->GetName()) == 0)
    {
      strcpy(toReturn, myInformations[i]->GetValue());
      return;
    }
  }

  for (int i = 0; i < myDOM->FindNumberOfElements("Information", myDomain->GetElement()); i++)
  {
    XdmfInformation currInfo = XdmfInformation();
    currInfo.SetDOM(myDOM);
    currInfo.SetElement(myDOM->FindElement("Information", i, myDomain->GetElement(), 0));
    currInfo.UpdateInformation();
    if (strcmp(informationName, currInfo.GetName()) == 0)
    {
      strcpy(toReturn, currInfo.GetValue());
      return;
    }
  }
}

/**
 *
 * Return the currentTime
 *
 */
void
XdmfFortran::GetTime(XdmfFloat64 * toReturn)
{
  *toReturn = currentTime;
}

/**
 *
 * Add a grid to the XdmfDOM.  Assign the current topology, geometry, and grid attributes
 * to grid.  If within a collection, add grid to the collection, otherwise
 * add to the top level domain.  Assign time value if value is nonnegative.
 *
 */
void
XdmfFortran::WriteGrid(char * gridName)
{
  XdmfGrid * grid = new XdmfGrid();

  if (myTopology == NULL)
  {
    cout << "Must set a topology before the grid can be written" << endl;
    delete grid;
    return;
  }

  if (myGeometry == NULL)
  {
    cout << "Must set a geometry before the grid can be written" << endl;
    delete grid;
    return;
  }

  // If we try to write over the same grid, modify the grid name...
  std::stringstream totalGridName;
  if (myGridNames.find(gridName) == myGridNames.end())
  {
    myGridNames[gridName] = 1;
    totalGridName << gridName;
  }
  else
  {
    myGridNames[gridName]++;
    totalGridName << gridName << "_" << myGridNames[gridName];
  }

  grid->SetName(totalGridName.str().c_str());

  // Set Topology
  if (myTopology->GetClass() != XDMF_STRUCTURED && myTopology->GetTopologyType() != XDMF_POLYVERTEX)
  {
    //Modify HDF5 names so we aren't writing over top of our data!
    std::stringstream topologyDataName;
    topologyDataName << myName << ".h5:/" << totalGridName.str() << "/Connections";
    myTopology->GetConnectivity()->SetHeavyDataSetName(topologyDataName.str().c_str());
  }
  grid->SetTopology(myTopology);

  // Set Geometry
  std::stringstream geometryDataName;
  geometryDataName << myName << ".h5:/" << totalGridName.str() << "/XYZ";
  myGeometry->GetPoints()->SetHeavyDataSetName(geometryDataName.str().c_str());
  grid->SetGeometry(myGeometry);

  if (!myCollections.empty())
  {
    myCollections.top()->Insert(grid);
  }
  else
  {
    myDomain->Insert(grid);
  }

  XdmfTime * t = new XdmfTime();
  if (currentTime >= 0)
  {
    t->SetTimeType(XDMF_TIME_SINGLE);
    t->SetValue(currentTime);
    grid->Insert(t);
    currentTime = -1;
  }

  while (myInformations.size() > 0)
  {
    grid->Insert(myInformations.back());
    myInformations.pop_back();
  }

  while (myAttributes.size() > 0)
  {
    XdmfAttribute * currAttribute = myAttributes.back();
    std::stringstream attributeDataName;
    attributeDataName << myName << ".h5:/" << totalGridName.str() << "/"
        << currAttribute->GetName();
    currAttribute->GetValues()->SetHeavyDataSetName(attributeDataName.str().c_str());

    grid->Insert(currAttribute);
    myAttributes.pop_back();
  }

  grid->Build();

  myGridPaths.push_back(myDOM->GetPath(grid->GetElement()));

  // If we are within a collection this will be deleted on collection deletion
  if (myCollections.empty())
  {
    delete grid;
  }
  myTopology = NULL;
  myGeometry = NULL;
}

/**
 *
 * Write constructed Xdmf file to disk with filename created upon initialization
 *
 */
void
XdmfFortran::WriteToFile()
{
  std::stringstream dataName;
  dataName << myName << ".xmf";
  myDOM->Write(dataName.str().c_str());
}

/**
 *
 * Print current XdmfDOM to console
 *
 */
void
XdmfFortran::Serialize()
{
  cout << myDOM->Serialize() << endl;
}

/**
 *
 * Copy current XdmfDOM to memory pointed to by charPointer
 *
 */
void
XdmfFortran::GetDOM(char * charPointer)
{
  strcpy(charPointer, myDOM->Serialize());
}

/**
 *
 * Helper function to write different datatypes to an XdmfArray.
 *
 */
void
XdmfFortran::WriteToXdmfArray(XdmfArray * array, XdmfPointer * data)
{
  switch (array->GetNumberType())
    {
  case XDMF_INT8_TYPE:
    array->SetValues(0, (XdmfInt8*) data, array->GetNumberOfElements());
    return;
  case XDMF_INT16_TYPE:
    array->SetValues(0, (XdmfInt16*) data, array->GetNumberOfElements());
    return;
  case XDMF_INT32_TYPE:
    array->SetValues(0, (XdmfInt32*) data, array->GetNumberOfElements());
    return;
  case XDMF_INT64_TYPE:
    array->SetValues(0, (XdmfInt64*) data, array->GetNumberOfElements());
    return;
  case XDMF_FLOAT32_TYPE:
    array->SetValues(0, (XdmfFloat32*) data, array->GetNumberOfElements());
    return;
  case XDMF_FLOAT64_TYPE:
    array->SetValues(0, (XdmfFloat64*) data, array->GetNumberOfElements());
    return;
  case XDMF_UINT8_TYPE:
    array->SetValues(0, (XdmfUInt8*) data, array->GetNumberOfElements());
    return;
  case XDMF_UINT16_TYPE:
    array->SetValues(0, (XdmfUInt16*) data, array->GetNumberOfElements());
    return;
  case XDMF_UINT32_TYPE:
    array->SetValues(0, (XdmfUInt32*) data, array->GetNumberOfElements());
    return;
  default:
    array->SetValues(0, (XdmfFloat64*) data, array->GetNumberOfElements());
    return;
    }
}

/**
 *
 * Helper function to read different datatypes from an XdmfArray.
 *
 */
void
XdmfFortran::ReadFromXdmfArray(XdmfArray * array, char * numberType, XdmfInt32 * startIndex,
    XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride,
    XdmfInt32 * valuesStride)
{
  XdmfArray a = XdmfArray();
  a.SetNumberTypeFromString(numberType);
  switch (a.GetNumberType())
    {
  case XDMF_INT8_TYPE:
    array->GetValues(*startIndex, (XdmfInt8*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_INT16_TYPE:
    array->GetValues(*startIndex, (XdmfInt16*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_INT32_TYPE:
    array->GetValues(*startIndex, (XdmfInt32*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_INT64_TYPE:
    array->GetValues(*startIndex, (XdmfInt64*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_FLOAT32_TYPE:
    array->GetValues(*startIndex, (XdmfFloat32*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_FLOAT64_TYPE:
    array->GetValues(*startIndex, (XdmfFloat64*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_UINT8_TYPE:
    array->GetValues(*startIndex, (XdmfUInt8*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_UINT16_TYPE:
    array->GetValues(*startIndex, (XdmfUInt16*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  case XDMF_UINT32_TYPE:
    array->GetValues(*startIndex, (XdmfUInt32*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
  default:
    array->GetValues(*startIndex, (XdmfFloat64*) arrayToFill, *numberOfValues, *arrayStride,
        *valuesStride);
    return;
    }
}

//
// C++ will mangle the name based on the argument list. This tells the
// compiler not to mangle the name so we can call it from 'C' (but
// really Fortran in this case)
//
extern "C"
{

  /**
   *
   * Initialize a new Xdmf file.
   *
   */
  void XDMF_UTILS_DLL
  XdmfInit(long * pointer, char * outputName)
  {
    printf("Inside XdmfInit - outputName is: %s\n", outputName);

    XdmfFortran * myPointer = new XdmfFortran(outputName);
    *pointer = (long) myPointer;
  }

  void XDMF_UTILS_DLL
  XdmfSetTime(long * pointer, double * t)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->SetTime(t);
  }

  void XDMF_UTILS_DLL
  XdmfAddCollection(long * pointer, char * collectionType)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddCollection(collectionType);
  }

  void XDMF_UTILS_DLL
  XdmfCloseCollection(long * pointer)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->CloseCollection();
  }

  void XDMF_UTILS_DLL
  XdmfSetGridTopology(long * pointer, char * topologyType, int * numberOfElements,
      XdmfInt32 * conns)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->SetGridTopology(topologyType, numberOfElements, conns);
  }

  void XDMF_UTILS_DLL
  XdmfSetGridTopologyFromShape(long * pointer, char * topologyType, char * shape, XdmfInt32 * conns)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->SetGridTopologyFromShape(topologyType, shape, conns);
  }

  void XDMF_UTILS_DLL
  XdmfSetGridGeometry(long * pointer, char * geometryType, char * numberType, int * numberOfPoints,
      XdmfPointer * points)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->SetGridGeometry(geometryType, numberType, numberOfPoints, points);
  }

  void XDMF_UTILS_DLL
  XdmfAddGridAttribute(long * pointer, char * attributeName, char * numberType,
      char * attributeCenter, char * attributeType, int * numberOfPoints, XdmfPointer * data)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddGridAttribute(attributeName, numberType, attributeCenter, attributeType,
        numberOfPoints, data);
  }

  void XDMF_UTILS_DLL
  XdmfAddGridAttributeFromShape(long * pointer, char * attributeName, char * numberType,
      char * attributeCenter, char * attributeType, char * shape, char * units, XdmfPointer * data)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddGridAttributeFromShape(attributeName, numberType, attributeCenter, attributeType,
        shape, units, data);
  }

  void XDMF_UTILS_DLL
  XdmfAddCollectionAttribute(long * pointer, char * attributeName, char * numberType,
      char * attributeCenter, char * attributeType, int * numberOfPoints, XdmfPointer * data)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddCollectionAttribute(attributeName, numberType, attributeCenter, attributeType,
        numberOfPoints, data);
  }

  void XDMF_UTILS_DLL
  XdmfAddGridInformation(long * pointer, char * informationName, char * value)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddGridInformation(informationName, value);
  }

  void XDMF_UTILS_DLL
  XdmfAddCollectionInformation(long * pointer, char * informationName, char * value)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddCollectionInformation(informationName, value);
  }

  void XDMF_UTILS_DLL
  XdmfAddArray(long * pointer, char * name, char * numberType, int * numberOfValues,
      XdmfPointer * data)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->AddArray(name, numberType, numberOfValues, data);
  }

  void XDMF_UTILS_DLL
  XdmfReadFile(long * pointer, char * filePath)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadFile(filePath);
  }

  void XDMF_UTILS_DLL
  XdmfReadGrid(long * pointer, char * gridName)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadGrid(gridName);
  }

  void XDMF_UTILS_DLL
  XdmfReadGridAtIndex(long * pointer, int * gridIndex)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadGridAtIndex(gridIndex);
  }

  void XDMF_UTILS_DLL
  XdmfGetNumberOfGrids(long * pointer, XdmfInt32 * toReturn)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->GetNumberOfGrids(toReturn);
  }

  void XDMF_UTILS_DLL
  XdmfGetNumberOfPoints(long * pointer, XdmfInt32 * toReturn)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->GetNumberOfPoints(toReturn);
  }

  void XDMF_UTILS_DLL
  XdmfReadPointValues(long * pointer, char * numberType, XdmfInt32 * startIndex,
      XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride,
      XdmfInt32 * valuesStride)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadPointValues(numberType, startIndex, arrayToFill, numberOfValues, arrayStride,
        valuesStride);
  }

  void XDMF_UTILS_DLL
  XdmfGetNumberOfAttributeValues(long * pointer, char * attributeName, XdmfInt32 * toReturn)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->GetNumberOfAttributeValues(attributeName, toReturn);
  }

  void XDMF_UTILS_DLL
  XdmfReadAttributeValues(long * pointer, char * attributeName, char * numberType,
      XdmfInt32 * startIndex, XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues,
      XdmfInt32 * arrayStride, XdmfInt32 * valuesStride)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadAttributeValues(attributeName, numberType, startIndex, arrayToFill,
        numberOfValues, arrayStride, valuesStride);
  }

  void XDMF_UTILS_DLL
  XdmfReadInformationValue(long * pointer, char * informationName, char * valueToReturn)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->ReadInformationValue(informationName, valueToReturn);
  }

  void XDMF_UTILS_DLL
  XdmfGetTime(long * pointer, XdmfFloat64 * toReturn)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->GetTime(toReturn);
  }

  void XDMF_UTILS_DLL
  XdmfWriteGrid(long * pointer, char * gridName)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->WriteGrid(gridName);
  }

  void XDMF_UTILS_DLL
  XdmfWriteToFile(long * pointer)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->WriteToFile();
  }

  void XDMF_UTILS_DLL
  XdmfSerialize(long * pointer)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->Serialize();
  }

  void XDMF_UTILS_DLL
  XdmfGetDOM(long * pointer, char * charPointer)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    myPointer->GetDOM(charPointer);
  }

  /**
   *
   * Close XdmfFortran interface and clean memory
   *
   */
  void XDMF_UTILS_DLL
  XdmfClose(long * pointer)
  {
    XdmfFortran * myPointer = (XdmfFortran *) *pointer;
    delete myPointer;
  }
}
