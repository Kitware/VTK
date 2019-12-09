/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfHeavyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXdmfHeavyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedIds.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXdmfDataArray.h"
#include "vtkXdmfReader.h"
#include "vtkXdmfReaderInternal.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <numeric>
#include <type_traits>
#include <vector>

#include "vtk_libxml2.h"
#include VTKLIBXML2_HEADER(tree.h)

#ifdef VTK_USE_64BIT_IDS
typedef XdmfInt64 vtkXdmfIdType;
#else
typedef XdmfInt32 vtkXdmfIdType;
#endif

using namespace xdmf2;

static void vtkScaleExtents(int in_exts[6], int out_exts[6], int stride[3])
{
  out_exts[0] = in_exts[0] / stride[0];
  out_exts[1] = in_exts[1] / stride[0];
  out_exts[2] = in_exts[2] / stride[1];
  out_exts[3] = in_exts[3] / stride[1];
  out_exts[4] = in_exts[4] / stride[2];
  out_exts[5] = in_exts[5] / stride[2];
}

static void vtkGetDims(int exts[6], int dims[3])
{
  dims[0] = exts[1] - exts[0] + 1;
  dims[1] = exts[3] - exts[2] + 1;
  dims[2] = exts[5] - exts[4] + 1;
}

//----------------------------------------------------------------------------
vtkXdmfHeavyData::vtkXdmfHeavyData(vtkXdmfDomain* domain, vtkAlgorithm* reader)
{
  this->Reader = reader;
  this->Piece = 0;
  this->NumberOfPieces = 0;
  this->GhostLevels = 0;
  this->Extents[0] = this->Extents[2] = this->Extents[4] = 0;
  this->Extents[1] = this->Extents[3] = this->Extents[5] = -1;
  this->Domain = domain;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
}

//----------------------------------------------------------------------------
vtkXdmfHeavyData::~vtkXdmfHeavyData() {}

//----------------------------------------------------------------------------
vtkDataObject* vtkXdmfHeavyData::ReadData()
{
  if (this->Domain->GetNumberOfGrids() == 1)
  {
    // There's just 1 grid. Now in serial, this is all good. In parallel, we
    // need to be care:
    // 1. If the data is structured, we respect the update-extent and read
    //    accordingly.
    // 2. If the data is unstructrued, we read only on the root node. The user
    //    can apply D3 or something to repartition the data.
    return this->ReadData(this->Domain->GetGrid(0));
  }

  // this code is similar to ReadComposite() however we cannot use the same code
  // since the API for getting the children differs on the domain and the grid.

  bool distribute_leaf_nodes = this->NumberOfPieces > 1;
  XdmfInt32 numChildren = this->Domain->GetNumberOfGrids();
  int number_of_leaf_nodes = 0;

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
  mb->SetNumberOfBlocks(numChildren);

  for (XdmfInt32 cc = 0; cc < numChildren; cc++)
  {
    XdmfGrid* xmfChild = this->Domain->GetGrid(cc);
    mb->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), xmfChild->GetName());
    bool child_is_leaf = (xmfChild->IsUniform() != 0);
    if (!child_is_leaf || !distribute_leaf_nodes ||
      (number_of_leaf_nodes % this->NumberOfPieces) == this->Piece)
    {
      // it's possible that the data has way too many blocks, in which case the
      // reader didn't present the user with capabilities to select the actual
      // leaf node blocks as is the norm, instead only top-level grids were
      // shown. In that case we need to ensure that we skip grids the user
      // wanted us to skip explicitly.
      if (!this->Domain->GetGridSelection()->ArrayIsEnabled(xmfChild->GetName()))
      {
        continue;
      }
      vtkDataObject* childDO = this->ReadData(xmfChild);
      if (childDO)
      {
        mb->SetBlock(cc, childDO);
        childDO->Delete();
      }
    }
    number_of_leaf_nodes += child_is_leaf ? 1 : 0;
  }

  return mb;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXdmfHeavyData::ReadData(XdmfGrid* xmfGrid, int blockId)
{
  if (!xmfGrid || xmfGrid->GetGridType() == XDMF_GRID_UNSET)
  {
    // sanity check-ensure that the xmfGrid is valid.
    return 0;
  }

  XdmfInt32 gridType = (xmfGrid->GetGridType() & XDMF_GRID_MASK);
  if (gridType == XDMF_GRID_COLLECTION &&
    xmfGrid->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL)
  {
    // grid is a temporal collection, pick the sub-grid with matching time and
    // process that.
    return this->ReadTemporalCollection(xmfGrid, blockId);
  }
  else if (gridType == XDMF_GRID_COLLECTION || gridType == XDMF_GRID_TREE)
  {
    return this->ReadComposite(xmfGrid);
  }

  // grid is a primitive grid, so read the data.
  return this->ReadUniformData(xmfGrid, blockId);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXdmfHeavyData::ReadComposite(XdmfGrid* xmfComposite)
{
  assert(((xmfComposite->GetGridType() & XDMF_GRID_COLLECTION &&
            xmfComposite->GetCollectionType() != XDMF_GRID_COLLECTION_TEMPORAL) ||
           (xmfComposite->GetGridType() & XDMF_GRID_TREE)) &&
    "Input must be a spatial collection or a tree");

  vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::New();
  XdmfInt32 numChildren = xmfComposite->GetNumberOfChildren();
  multiBlock->SetNumberOfBlocks(numChildren);

  bool distribute_leaf_nodes =
    (xmfComposite->GetGridType() & XDMF_GRID_COLLECTION && this->NumberOfPieces > 1);

  int number_of_leaf_nodes = 0;
  for (XdmfInt32 cc = 0; cc < numChildren; cc++)
  {
    XdmfGrid* xmfChild = xmfComposite->GetChild(cc);
    multiBlock->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), xmfChild->GetName());
    bool child_is_leaf = (xmfChild->IsUniform() != 0);
    if (!child_is_leaf || !distribute_leaf_nodes ||
      (number_of_leaf_nodes % this->NumberOfPieces) == this->Piece)
    {
      vtkDataObject* childDO = this->ReadData(xmfChild, cc);
      if (childDO)
      {
        multiBlock->SetBlock(cc, childDO);
        childDO->Delete();
      }
    }
    number_of_leaf_nodes += child_is_leaf ? 1 : 0;
  }

  return multiBlock;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXdmfHeavyData::ReadTemporalCollection(
  XdmfGrid* xmfTemporalCollection, int blockId)
{
  assert(xmfTemporalCollection->GetGridType() & XDMF_GRID_COLLECTION &&
    xmfTemporalCollection->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL &&
    "Input must be a temporal collection");

  // Find the children that are valid for the requested time (this->Time) and
  // read only those.

  // FIXME: I am tempted to remove support for supporting multiple matching
  // sub-grids for a time-step since that changes the composite data hierarchy
  // over time which makes it hard to use filters such as vtkExtractBlock etc.

  std::deque<XdmfGrid*> valid_children;
  for (XdmfInt32 cc = 0; cc < xmfTemporalCollection->GetNumberOfChildren(); cc++)
  {
    XdmfGrid* child = xmfTemporalCollection->GetChild(cc);
    if (child)
    {
      // ensure that we set correct epsilon for comparison
      // BUG #0013766.
      child->GetTime()->SetEpsilon(VTK_DBL_EPSILON);
      if (child->GetTime()->IsValid(this->Time, this->Time))
      {
        valid_children.push_back(child);
      }
    }
  }
  // if no child matched this timestep, handle the case where the user didn't
  // specify any <Time /> element for the temporal collection.
  for (XdmfInt32 cc = 0;
       valid_children.size() == 0 && cc < xmfTemporalCollection->GetNumberOfChildren(); cc++)
  {
    XdmfGrid* child = xmfTemporalCollection->GetChild(cc);
    if (child && child->GetTime()->GetTimeType() == XDMF_TIME_UNSET)
    {
      valid_children.push_back(child);
    }
  }

  if (valid_children.size() == 0)
  {
    return 0;
  }

  std::deque<vtkSmartPointer<vtkDataObject> > child_data_objects;
  std::deque<XdmfGrid*>::iterator iter;
  for (iter = valid_children.begin(); iter != valid_children.end(); ++iter)
  {
    vtkDataObject* childDO = this->ReadData(*iter, blockId);
    if (childDO)
    {
      child_data_objects.push_back(childDO);
      childDO->Delete();
    }
  }

  if (child_data_objects.size() == 1)
  {
    vtkDataObject* dataObject = child_data_objects[0];
    dataObject->Register(nullptr);
    return dataObject;
  }
  else if (child_data_objects.size() > 1)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(static_cast<unsigned int>(child_data_objects.size()));
    for (unsigned int cc = 0; cc < static_cast<unsigned int>(child_data_objects.size()); cc++)
    {
      mb->SetBlock(cc, child_data_objects[cc]);
    }
    return mb;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Read a non-composite grid. Note here uniform has nothing to do with
// vtkUniformGrid but to what Xdmf's GridType="Uniform".
vtkDataObject* vtkXdmfHeavyData::ReadUniformData(XdmfGrid* xmfGrid, int blockId)
{
  assert(xmfGrid->IsUniform() && "Input must be a uniform xdmf grid.");

  int vtk_data_type = this->Domain->GetVTKDataType(xmfGrid);

  if (!this->Domain->GetGridSelection()->ArrayIsEnabled(xmfGrid->GetName()))
  {
    // simply create an empty data-object of the correct type and return it.
    return vtkDataObjectTypes::NewDataObject(vtk_data_type);
  }

  // Read heavy data for grid geometry/topology. This does not read any
  // data-arrays. They are read explicitly.
  XdmfTopology* topo = xmfGrid->GetTopology();
  XdmfGeometry* geom = xmfGrid->GetGeometry();
  xmlChar* filePtr;

  bool caching = true;
  XdmfDOM* topoDom = topo->GetDOM();
  XdmfXmlNode topoNode = topo->GetElement();
  XdmfXmlNode topoNodeDataItem = topoDom->FindElement("DataItem", 0, topoNode);
  std::string topoFilename = "NULL";
  if (topoNodeDataItem && caching)
  {
    filePtr = topoNodeDataItem->children->content;
    if (filePtr != nullptr)
    {
      topoFilename = reinterpret_cast<char*>(filePtr);
    }
    else
    {
      // vtkErrorWithObjectMacro(this->Reader, << "Cannot find DataItem element in topology xml, no
      // caching possible");
      caching = false;
    }
  }
  else
  {
    caching = false;
  }

  XdmfDOM* geomDom = geom->GetDOM();
  XdmfXmlNode geomNode = geom->GetElement();
  XdmfXmlNode geomNodeDataItem = geomDom->FindElement("DataItem", 0, geomNode);
  std::string geomFilename = "NULL";
  if (geomNodeDataItem && caching)
  {
    filePtr = geomNodeDataItem->children->content;
    if (filePtr != nullptr)
    {
      geomFilename = reinterpret_cast<char*>(filePtr);
    }
    else
    {
      vtkErrorWithObjectMacro(
        this->Reader, << "Cannot find DataItem element in geometry xml, no caching possible");
      caching = false;
    }
  }
  else
  {
    caching = false;
  }

  vtkXdmfReader::XdmfReaderCachedData& cache =
    vtkXdmfReader::SafeDownCast(this->Reader)->GetDataSetCache();
  vtkXdmfReader::XdmfDataSetTopoGeoPath& cachedData = cache[blockId];
  if (caching && (cachedData.topologyPath == topoFilename) &&
    (cachedData.geometryPath == geomFilename))
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(
      vtkDataObjectTypes::NewDataObject(cachedData.dataset->GetDataObjectType()));
    ds->ShallowCopy(cachedData.dataset);
    this->ReadAttributes(ds, xmfGrid);
    return ds;
  }

  if (caching)
  {
    cachedData.topologyPath = topoFilename;
    cachedData.geometryPath = geomFilename;
    if (cache[blockId].dataset != nullptr)
    {
      cache[blockId].dataset->Delete();
      cache[blockId].dataset = nullptr;
    }
  }

  XdmfInt32 status = xmfGrid->Update();
  if (status == XDMF_FAIL)
  {
    return 0;
  }

  vtkDataObject* dataObject = 0;

  switch (vtk_data_type)
  {
    case VTK_UNIFORM_GRID:
      dataObject = this->RequestImageData(xmfGrid, true);
      break;

    case VTK_IMAGE_DATA:
      dataObject = this->RequestImageData(xmfGrid, false);
      break;

    case VTK_STRUCTURED_GRID:
      dataObject = this->RequestStructuredGrid(xmfGrid);
      break;

    case VTK_RECTILINEAR_GRID:
      dataObject = this->RequestRectilinearGrid(xmfGrid);
      break;

    case VTK_UNSTRUCTURED_GRID:
      dataObject = this->ReadUnstructuredGrid(xmfGrid);
      break;

    default:
      // un-handled case.
      return 0;
  }

  if (caching)
  {
    cache[blockId].dataset = vtkDataSet::SafeDownCast(dataObject);
    dataObject->Register(0);
  }
  return dataObject;
}

//----------------------------------------------------------------------------
int vtkXdmfHeavyData::GetNumberOfPointsPerCell(int vtk_cell_type)
{
  switch (vtk_cell_type)
  {
    case VTK_POLY_VERTEX:
      return 0;
    case VTK_POLY_LINE:
      return 0;
    case VTK_POLYGON:
      return 0;

    case VTK_TRIANGLE:
      return 3;
    case VTK_QUAD:
      return 4;
    case VTK_TETRA:
      return 4;
    case VTK_PYRAMID:
      return 5;
    case VTK_WEDGE:
      return 6;
    case VTK_HEXAHEDRON:
      return 8;
    case VTK_QUADRATIC_EDGE:
      return 3;
    case VTK_QUADRATIC_TRIANGLE:
      return 6;
    case VTK_QUADRATIC_QUAD:
      return 8;
    case VTK_BIQUADRATIC_QUAD:
      return 9;
    case VTK_QUADRATIC_TETRA:
      return 10;
    case VTK_QUADRATIC_PYRAMID:
      return 13;
    case VTK_QUADRATIC_WEDGE:
      return 15;
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return 18;
    case VTK_QUADRATIC_HEXAHEDRON:
      return 20;
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      return 24;
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      return 24;
  }
  return -1;
}
//----------------------------------------------------------------------------
int vtkXdmfHeavyData::GetVTKCellType(XdmfInt32 topologyType)
{
  switch (topologyType)
  {
    case XDMF_POLYVERTEX:
      return VTK_POLY_VERTEX;
    case XDMF_POLYLINE:
      return VTK_POLY_LINE;
    case XDMF_POLYGON:
      return VTK_POLYGON; // FIXME: should this not be treated as mixed?
    case XDMF_TRI:
      return VTK_TRIANGLE;
    case XDMF_QUAD:
      return VTK_QUAD;
    case XDMF_TET:
      return VTK_TETRA;
    case XDMF_PYRAMID:
      return VTK_PYRAMID;
    case XDMF_WEDGE:
      return VTK_WEDGE;
    case XDMF_HEX:
      return VTK_HEXAHEDRON;
    case XDMF_EDGE_3:
      return VTK_QUADRATIC_EDGE;
    case XDMF_TRI_6:
      return VTK_QUADRATIC_TRIANGLE;
    case XDMF_QUAD_8:
      return VTK_QUADRATIC_QUAD;
    case XDMF_QUAD_9:
      return VTK_BIQUADRATIC_QUAD;
    case XDMF_TET_10:
      return VTK_QUADRATIC_TETRA;
    case XDMF_PYRAMID_13:
      return VTK_QUADRATIC_PYRAMID;
    case XDMF_WEDGE_15:
      return VTK_QUADRATIC_WEDGE;
    case XDMF_WEDGE_18:
      return VTK_BIQUADRATIC_QUADRATIC_WEDGE;
    case XDMF_HEX_20:
      return VTK_QUADRATIC_HEXAHEDRON;
    case XDMF_HEX_24:
      return VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON;
    case XDMF_HEX_27:
      return VTK_TRIQUADRATIC_HEXAHEDRON;
    case XDMF_MIXED:
      return VTK_NUMBER_OF_CELL_TYPES;
  }
  // XdmfErrorMessage("Unknown Topology Type = "
  //  << xmfGrid->GetTopology()->GetTopologyType());
  return VTK_EMPTY_CELL;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXdmfHeavyData::ReadUnstructuredGrid(XdmfGrid* xmfGrid)
{
  vtkSmartPointer<vtkUnstructuredGrid> ugData = vtkSmartPointer<vtkUnstructuredGrid>::New();

  // BUG #12527. For non-partitioned data, don't read unstructured grid on
  // process id > 0.
  if (this->Piece != 0 && this->Domain->GetNumberOfGrids() == 1 &&
    this->Domain->GetVTKDataType() == VTK_UNSTRUCTURED_GRID &&
    this->Domain->GetSetsSelection()->GetNumberOfArrays() == 0)
  {
    ugData->Register(nullptr);
    return ugData;
  }

  XdmfTopology* xmfTopology = xmfGrid->GetTopology();
  XdmfArray* xmfConnectivity = xmfTopology->GetConnectivity();

  int vtk_cell_type = vtkXdmfHeavyData::GetVTKCellType(xmfTopology->GetTopologyType());

  if (vtk_cell_type == VTK_EMPTY_CELL)
  {
    // invalid topology.
    return nullptr;
  }

  if (vtk_cell_type != VTK_NUMBER_OF_CELL_TYPES)
  // i.e. topologyType != XDMF_MIXED
  {
    // all cells are of the same type.
    XdmfInt32 numPointsPerCell = xmfTopology->GetNodesPerElement();

    // FIXME: is this needed, shouldn't xmfTopology->GetNodesPerElement()
    // return the correct value always?
    if (xmfConnectivity->GetRank() == 2)
    {
      numPointsPerCell = xmfConnectivity->GetDimension(1);
    }

    /* Create Cell Type Array */
    XdmfInt64 conn_length = xmfConnectivity->GetNumberOfElements();
    std::vector<XdmfInt64> xmfConnections(conn_length);
    xmfConnectivity->GetValues(0, xmfConnections.data(), conn_length);

    vtkIdType numCells = xmfTopology->GetShapeDesc()->GetNumberOfElements();

    vtkNew<vtkIdTypeArray> conn;
    vtkNew<vtkIdTypeArray> offsets;

    offsets->SetNumberOfTuples(numCells + 1);

    { // Fill offsets: {0, 1 * cellSize, 2 * cellSize, ..., numCells * cellSize}
      vtkIdType offset = -numPointsPerCell;
      auto generator = [&]() -> vtkIdType { return offset += numPointsPerCell; };
      auto range = vtk::DataArrayValueRange<1>(offsets);
      std::generate(range.begin(), range.end(), generator);
    }

    conn->SetNumberOfTuples(numPointsPerCell * numCells);

    // Fill connections (just copy xmfConnections)
    { // Need to convert explicitly to silence warnings:
      auto range = vtk::DataArrayValueRange<1>(conn);
      std::transform(xmfConnections.cbegin(),
        xmfConnections.cbegin() + (numPointsPerCell * numCells), range.begin(),
        [](XdmfInt64 val) -> vtkIdType { return static_cast<vtkIdType>(val); });
    }

    // Construct and set the cell array
    vtkNew<vtkCellArray> cells;
    cells->SetData(offsets, conn);
    ugData->SetCells(vtk_cell_type, cells);
  }
  else
  {
    // We have cells with mixed types.
    XdmfInt64 conn_length = xmfGrid->GetTopology()->GetConnectivity()->GetNumberOfElements();
    std::vector<XdmfInt64> xmfConnections(static_cast<size_t>(conn_length));
    xmfConnectivity->GetValues(0, xmfConnections.data(), conn_length);

    vtkIdType numCells = xmfTopology->GetShapeDesc()->GetNumberOfElements();
    vtkNew<vtkUnsignedCharArray> cell_types;
    cell_types->SetNumberOfTuples(numCells);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(numCells + 1);

    vtkNew<vtkIdTypeArray> conn;
    // This may be an overestimate; will correct after filling.
    conn->SetNumberOfTuples(static_cast<vtkIdType>(conn_length));

    vtkIdType offset = 0;
    vtkIdType index = 0;
    vtkIdType connIndex = 0;
    for (vtkIdType cc = 0; cc < numCells; cc++)
    {
      int vtk_cell_typeI = this->GetVTKCellType(xmfConnections[index++]);
      XdmfInt32 numPointsPerCell = this->GetNumberOfPointsPerCell(vtk_cell_typeI);
      if (numPointsPerCell == -1)
      {
        // encountered an unknown cell.
        return nullptr;
      }

      if (numPointsPerCell == 0)
      {
        // cell type does not have a fixed number of points in which case the
        // next entry in xmfConnections tells us the number of points.
        numPointsPerCell = xmfConnections[index++];
      }

      cell_types->SetValue(cc, static_cast<unsigned char>(vtk_cell_typeI));
      offsets->SetValue(cc, offset);
      offset += numPointsPerCell;

      for (vtkIdType i = 0; i < numPointsPerCell; i++)
      {
        conn->SetValue(connIndex++, xmfConnections[index++]);
      }
    }
    offsets->SetValue(numCells, offset); // final offset value

    // Resize the Array to the Proper Size
    conn->Resize(connIndex);

    // Create and set the cell array:
    vtkNew<vtkCellArray> cells;
    cells->SetData(offsets, conn);
    ugData->SetCells(cell_types, cells);
  }

  // Read the geometry.
  vtkPoints* points = this->ReadPoints(xmfGrid->GetGeometry());
  if (!points)
  {
    // failed to read points.
    return nullptr;
  }
  ugData->SetPoints(points);
  points->Delete();

  this->ReadAttributes(ugData, xmfGrid);

  // Read ghost cell/point information.
  this->ReadGhostSets(ugData, xmfGrid);

  // If this grid has sets defined on it, then we need to read those as well
  vtkMultiBlockDataSet* sets = this->ReadSets(ugData, xmfGrid);
  if (sets)
  {
    return sets;
  }

  ugData->Register(nullptr);
  return ugData;
}

inline bool vtkExtentsAreValid(int exts[6])
{
  return exts[1] >= exts[0] && exts[3] >= exts[2] && exts[5] >= exts[4];
}

inline bool vtkExtentsAreEqual(int* exts1, int* exts2)
{
  if (!exts1 && !exts2)
  {
    return true;
  }
  if (!exts1 || !exts2)
  {
    return false;
  }
  return (exts1[0] == exts2[0] && exts1[1] == exts2[1] && exts1[2] == exts2[2] &&
    exts1[3] == exts2[3] && exts1[4] == exts2[4] && exts1[5] == exts2[5]);
}

//-----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXdmfHeavyData::RequestRectilinearGrid(XdmfGrid* xmfGrid)
{
  vtkSmartPointer<vtkRectilinearGrid> rg = vtkSmartPointer<vtkRectilinearGrid>::New();
  int whole_extents[6];
  int update_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  if (!vtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  // convert to stridden update extents.
  int scaled_extents[6];
  vtkScaleExtents(update_extents, scaled_extents, this->Stride);
  int scaled_dims[3];
  vtkGetDims(scaled_extents, scaled_dims);

  rg->SetExtent(scaled_extents);

  // Now read rectilinear geometry.
  XdmfGeometry* xmfGeometry = xmfGrid->GetGeometry();

  vtkSmartPointer<vtkDoubleArray> xarray = vtkSmartPointer<vtkDoubleArray>::New();
  xarray->SetNumberOfTuples(scaled_dims[0]);

  vtkSmartPointer<vtkDoubleArray> yarray = vtkSmartPointer<vtkDoubleArray>::New();
  yarray->SetNumberOfTuples(scaled_dims[1]);

  vtkSmartPointer<vtkDoubleArray> zarray = vtkSmartPointer<vtkDoubleArray>::New();
  zarray->SetNumberOfTuples(scaled_dims[2]);

  rg->SetXCoordinates(xarray);
  rg->SetYCoordinates(yarray);
  rg->SetZCoordinates(zarray);

  switch (xmfGeometry->GetGeometryType())
  {
    case XDMF_GEOMETRY_ORIGIN_DXDY:
    case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
    {
      XdmfFloat64* origin = xmfGeometry->GetOrigin();
      XdmfFloat64* dxdydz = xmfGeometry->GetDxDyDz();
      for (int cc = scaled_extents[0]; cc <= scaled_extents[1]; cc++)
      {
        xarray->GetPointer(0)[cc - scaled_extents[0]] =
          origin[0] + (dxdydz[0] * cc * this->Stride[0]);
      }
      for (int cc = scaled_extents[2]; cc <= scaled_extents[3]; cc++)
      {
        yarray->GetPointer(0)[cc - scaled_extents[2]] =
          origin[1] + (dxdydz[1] * cc * this->Stride[1]);
      }
      for (int cc = scaled_extents[4]; cc <= scaled_extents[5]; cc++)
      {
        zarray->GetPointer(0)[cc - scaled_extents[4]] =
          origin[2] + (dxdydz[2] * cc * this->Stride[2]);
      }
    }
    break;

    case XDMF_GEOMETRY_VXVY:
    {
      // Note:
      // XDMF and VTK structured extents are reversed
      // Where I varies fastest, VTK's convention is IJK, but XDMF's is KJI
      // However, users naturally don't want VXVY to mean VZVY.
      // Let's accept VisIt's interpretation of this 2D case
      // (KJI is ZXY where Z=0).
      xarray->SetNumberOfTuples(scaled_dims[1]);
      yarray->SetNumberOfTuples(scaled_dims[2]);
      zarray->SetNumberOfTuples(scaled_dims[0]);
      rg->SetExtent(scaled_extents[2], scaled_extents[3], scaled_extents[4], scaled_extents[5],
        scaled_extents[0], scaled_extents[1]);
      xmfGeometry->GetVectorX()->GetValues(
        update_extents[2], xarray->GetPointer(0), scaled_dims[1], this->Stride[1]);
      xmfGeometry->GetVectorY()->GetValues(
        update_extents[4], yarray->GetPointer(0), scaled_dims[2], this->Stride[2]);
      zarray->FillComponent(0, 0);
    }
    break;

    case XDMF_GEOMETRY_VXVYVZ:
    {
      xmfGeometry->GetVectorX()->GetValues(
        update_extents[0], xarray->GetPointer(0), scaled_dims[0], this->Stride[0]);
      xmfGeometry->GetVectorY()->GetValues(
        update_extents[2], yarray->GetPointer(0), scaled_dims[1], this->Stride[1]);
      xmfGeometry->GetVectorZ()->GetValues(
        update_extents[4], zarray->GetPointer(0), scaled_dims[2], this->Stride[2]);
    }
    break;

    default:
      vtkErrorWithObjectMacro(this->Reader,
        "Geometry type : " << xmfGeometry->GetGeometryTypeAsString() << " is not supported for "
                           << xmfGrid->GetTopology()->GetTopologyTypeAsString());
      return nullptr;
  }

  this->ReadAttributes(rg, xmfGrid, update_extents);
  rg->Register(nullptr);
  return rg;
}

//-----------------------------------------------------------------------------
vtkStructuredGrid* vtkXdmfHeavyData::RequestStructuredGrid(XdmfGrid* xmfGrid)
{
  vtkStructuredGrid* sg = vtkStructuredGrid::New();

  int whole_extents[6];
  int update_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  if (!vtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  int scaled_extents[6];
  vtkScaleExtents(update_extents, scaled_extents, this->Stride);
  sg->SetExtent(scaled_extents);

  vtkPoints* points = this->ReadPoints(xmfGrid->GetGeometry(), update_extents, whole_extents);
  sg->SetPoints(points);
  points->Delete();

  this->ReadAttributes(sg, xmfGrid, update_extents);
  return sg;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkXdmfHeavyData::RequestImageData(XdmfGrid* xmfGrid, bool use_uniform_grid)
{
  vtkImageData* imageData =
    use_uniform_grid ? static_cast<vtkImageData*>(vtkUniformGrid::New()) : vtkImageData::New();

  int whole_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  int update_extents[6];

  if (!vtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  int scaled_extents[6];
  vtkScaleExtents(update_extents, scaled_extents, this->Stride);
  imageData->SetExtent(scaled_extents);

  double origin[3], spacing[3];
  if (!this->Domain->GetOriginAndSpacing(xmfGrid, origin, spacing))
  {
    vtkErrorWithObjectMacro(this->Reader,
      "Could not determine image-data origin and spacing. "
      "Required geometry type is ORIGIN_DXDY or ORIGIN_DXDYDZ. "
      "The specified geometry type is : "
        << xmfGrid->GetGeometry()->GetGeometryTypeAsString());
    // release image data.
    imageData->Delete();
    return nullptr;
  }
  imageData->SetOrigin(origin);
  imageData->SetSpacing(
    spacing[0] * this->Stride[0], spacing[1] * this->Stride[1], spacing[2] * this->Stride[2]);
  this->ReadAttributes(imageData, xmfGrid, update_extents);
  return imageData;
}

//-----------------------------------------------------------------------------
vtkPoints* vtkXdmfHeavyData::ReadPoints(
  XdmfGeometry* xmfGeometry, int* update_extents /*=nullptr*/, int* whole_extents /*=nullptr*/)
{
  XdmfInt32 geomType = xmfGeometry->GetGeometryType();

  if (geomType != XDMF_GEOMETRY_X_Y_Z && geomType != XDMF_GEOMETRY_XYZ &&
    geomType != XDMF_GEOMETRY_X_Y && geomType != XDMF_GEOMETRY_XY)
  {
    return nullptr;
  }

  XdmfArray* xmfPoints = xmfGeometry->GetPoints();
  if (!xmfPoints)
  {
    XdmfErrorMessage("No Points to Set");
    return nullptr;
  }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  if (xmfPoints->GetNumberType() == XDMF_FLOAT32_TYPE)
  {
    vtkFloatArray* da = vtkFloatArray::New();
    da->SetNumberOfComponents(3);
    points->SetData(da);
    da->Delete();
  }
  else // means == XDMF_FLOAT64_TYPE
  {
    vtkDoubleArray* da = vtkDoubleArray::New();
    da->SetNumberOfComponents(3);
    points->SetData(da);
    da->Delete();
  }

  XdmfInt64 numGeometryPoints = xmfGeometry->GetNumberOfPoints();
  vtkIdType numPoints = numGeometryPoints;
  bool structured_data = false;
  if (update_extents && whole_extents)
  {
    // we are reading a sub-extent.
    structured_data = true;
    int scaled_extents[6];
    int scaled_dims[3];
    vtkScaleExtents(update_extents, scaled_extents, this->Stride);
    vtkGetDims(scaled_extents, scaled_dims);
    numPoints = (scaled_dims[0] * scaled_dims[1] * scaled_dims[2]);
  }
  points->SetNumberOfPoints(numPoints);

  if (!structured_data)
  {
    // read all the points.
    switch (points->GetData()->GetDataType())
    {
      case VTK_DOUBLE:
        xmfPoints->GetValues(
          0, reinterpret_cast<double*>(points->GetVoidPointer(0)), numPoints * 3);
        break;

      case VTK_FLOAT:
        xmfPoints->GetValues(0, reinterpret_cast<float*>(points->GetVoidPointer(0)), numPoints * 3);
        break;

      default:
        return nullptr;
    }
  }
  else
  {
    // treating the points as structured points
    std::vector<XdmfFloat64> tempPoints(numGeometryPoints * 3);
    xmfPoints->GetValues(0, tempPoints.data(), numGeometryPoints * 3);
    vtkIdType pointId = 0;
    int xdmf_dims[3];
    vtkGetDims(whole_extents, xdmf_dims);

    for (int z = update_extents[4]; z <= update_extents[5]; z++)
    {
      if ((z - update_extents[4]) % this->Stride[2])
      {
        continue;
      }

      for (int y = update_extents[2]; y <= update_extents[3]; y++)
      {
        if ((y - update_extents[2]) % this->Stride[1])
        {
          continue;
        }

        for (int x = update_extents[0]; x <= update_extents[1]; x++)
        {
          if ((x - update_extents[0]) % this->Stride[0])
          {
            continue;
          }

          int xdmf_index[3] = { x, y, z };
          XdmfInt64 offset = vtkStructuredData::ComputePointId(xdmf_dims, xdmf_index);
          points->SetPoint(pointId, tempPoints[3 * offset], tempPoints[3 * offset + 1],
            tempPoints[3 * offset + 2]);
          pointId++;
        }
      }
    }
  }

  points->Register(0);
  return points;
}

//-----------------------------------------------------------------------------
bool vtkXdmfHeavyData::ReadAttributes(vtkDataSet* dataSet, XdmfGrid* xmfGrid, int* update_extents)
{
  int data_dimensionality = this->Domain->GetDataDimensionality(xmfGrid);

  int numAttributes = xmfGrid->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfGrid->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (!attrName)
    {
      vtkWarningWithObjectMacro(this->Reader, "Skipping unnamed attributes.");
      continue;
    }

    vtkFieldData* fieldData = 0;
    // skip disabled arrays.
    switch (attrCenter)
    {
      case XDMF_ATTRIBUTE_CENTER_GRID:
        fieldData = dataSet->GetFieldData();
        break;

      case XDMF_ATTRIBUTE_CENTER_CELL:
        if (!this->Domain->GetCellArraySelection()->ArrayIsEnabled(attrName))
        {
          continue;
        }
        fieldData = dataSet->GetCellData();
        break;

      case XDMF_ATTRIBUTE_CENTER_NODE:
        if (!this->Domain->GetPointArraySelection()->ArrayIsEnabled(attrName))
        {
          continue;
        }
        fieldData = dataSet->GetPointData();
        break;

      case XDMF_ATTRIBUTE_CENTER_FACE:
      case XDMF_ATTRIBUTE_CENTER_EDGE:
      default:
        vtkWarningWithObjectMacro(this->Reader,
          "Skipping attribute " << attrName << " at "
                                << xmfAttribute->GetAttributeCenterAsString());
        continue; // unhandled.
    }

    vtkDataArray* array = this->ReadAttribute(xmfAttribute, data_dimensionality, update_extents);
    if (array)
    {
      array->SetName(attrName);
      fieldData->AddArray(array);
      bool is_active = xmfAttribute->GetActive() != 0;
      vtkDataSetAttributes* attributes = vtkDataSetAttributes::SafeDownCast(fieldData);
      if (attributes)
      {
        // make attribute active.
        switch (xmfAttribute->GetAttributeType())
        {
          case XDMF_ATTRIBUTE_TYPE_SCALAR:
            if (is_active || attributes->GetScalars() == nullptr)
            {
              attributes->SetActiveScalars(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_VECTOR:
            if (is_active || attributes->GetVectors() == nullptr)
            {
              attributes->SetActiveVectors(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_TENSOR:
          case XDMF_ATTRIBUTE_TYPE_TENSOR6:
            if (is_active || attributes->GetTensors() == nullptr)
            {
              attributes->SetActiveTensors(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_GLOBALID:
            if (is_active || attributes->GetGlobalIds() == nullptr)
            {
              attributes->SetActiveGlobalIds(attrName);
            }
        }
      }
      array->Delete();
    }
  }
  return true;
}

// used to convert a symmetric tensor to a regular tensor.
template <class T>
void vtkConvertTensor6(T* source, T* dest, vtkIdType numTensors)
{
  for (vtkIdType cc = 0; cc < numTensors; cc++)
  {
    dest[cc * 9 + 0] = source[cc * 6 + 0];
    dest[cc * 9 + 1] = source[cc * 6 + 1];
    dest[cc * 9 + 2] = source[cc * 6 + 2];

    dest[cc * 9 + 3] = source[cc * 6 + 1];
    dest[cc * 9 + 4] = source[cc * 6 + 3];
    dest[cc * 9 + 5] = source[cc * 6 + 4];

    dest[cc * 9 + 6] = source[cc * 6 + 2];
    dest[cc * 9 + 7] = source[cc * 6 + 4];
    dest[cc * 9 + 8] = source[cc * 6 + 5];
  }
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkXdmfHeavyData::ReadAttribute(
  XdmfAttribute* xmfAttribute, int data_dimensionality, int* update_extents /*=0*/)
{
  if (!xmfAttribute)
  {
    return nullptr;
  }

  int attrType = xmfAttribute->GetAttributeType();
  int attrCenter = xmfAttribute->GetAttributeCenter();
  int numComponents = 1;

  XdmfDataItem xmfDataItem;
  xmfDataItem.SetDOM(xmfAttribute->GetDOM());
  xmfDataItem.SetElement(xmfAttribute->GetDOM()->FindDataElement(0, xmfAttribute->GetElement()));
  xmfDataItem.UpdateInformation();

  XdmfInt64 data_dims[XDMF_MAX_DIMENSION];
  int data_rank = xmfDataItem.GetDataDesc()->GetShape(data_dims);

  switch (attrType)
  {
    case XDMF_ATTRIBUTE_TYPE_TENSOR:
      numComponents = 9;
      break;
    case XDMF_ATTRIBUTE_TYPE_TENSOR6:
      numComponents = 6;
      break;
    case XDMF_ATTRIBUTE_TYPE_VECTOR:
      numComponents = 3;
      break;
    default:
      numComponents = 1;
      break;
  }

  // Handle 2D vectors
  if (attrType == XDMF_ATTRIBUTE_TYPE_VECTOR && data_dims[data_rank - 1] == 2)
  {
    numComponents = 2;
  }

  if (update_extents && attrCenter != XDMF_ATTRIBUTE_CENTER_GRID)
  {
    // for hyperslab selection to work, the data shape must match the topology
    // shape.
    if (data_rank < 0)
    {
      vtkErrorWithObjectMacro(this->Reader, "Unsupported attribute rank: " << data_rank);
      return nullptr;
    }
    if (data_rank > (data_dimensionality + 1))
    {
      vtkErrorWithObjectMacro(
        this->Reader, "The data_dimensionality and topology dimensionality mismatch");
      return nullptr;
    }
    XdmfInt64 start[4] = { update_extents[4], update_extents[2], update_extents[0], 0 };
    XdmfInt64 stride[4] = { this->Stride[2], this->Stride[1], this->Stride[0], 1 };
    XdmfInt64 count[4] = { 0, 0, 0, 0 };
    int scaled_dims[3];
    int scaled_extents[6];
    vtkScaleExtents(update_extents, scaled_extents, this->Stride);
    vtkGetDims(scaled_extents, scaled_dims);
    count[0] = (scaled_dims[2] - 1);
    count[1] = (scaled_dims[1] - 1);
    count[2] = (scaled_dims[0] - 1);
    if (data_rank == (data_dimensionality + 1))
    {
      // this refers the number of components in the attribute.
      count[data_dimensionality] = data_dims[data_dimensionality];
    }

    if (attrCenter == XDMF_ATTRIBUTE_CENTER_NODE)
    {
      // Point count is 1 + cell extent if not a single layer
      count[0] += 1; //((update_extents[5] - update_extents[4]) > 0)? 1 : 0;
      count[1] += 1; //((update_extents[3] - update_extents[2]) > 0)? 1 : 0;
      count[2] += 1; //((update_extents[1] - update_extents[0]) > 0)? 1 : 0;
    }
    xmfDataItem.GetDataDesc()->SelectHyperSlab(start, stride, count);
  }

  if (xmfDataItem.Update() == XDMF_FAIL)
  {
    vtkErrorWithObjectMacro(this->Reader, "Failed to read attribute data");
    return 0;
  }

  vtkXdmfDataArray* xmfConvertor = vtkXdmfDataArray::New();
  vtkDataArray* dataArray = xmfConvertor->FromXdmfArray(
    xmfDataItem.GetArray()->GetTagName(), 1, data_rank, numComponents, 0);
  xmfConvertor->Delete();

  if (attrType == XDMF_ATTRIBUTE_TYPE_TENSOR6)
  {
    // convert Tensor6 to Tensor.
    vtkDataArray* tensor = dataArray->NewInstance();
    vtkIdType numTensors = dataArray->GetNumberOfTuples();
    tensor->SetNumberOfComponents(9);
    tensor->SetNumberOfTuples(numTensors);

    // Copy Symmetrical Tensor Values to Correct Positions in 3x3 matrix
    void* source = dataArray->GetVoidPointer(0);
    void* dest = tensor->GetVoidPointer(0);
    switch (tensor->GetDataType())
    {
      vtkTemplateMacro(vtkConvertTensor6(
        reinterpret_cast<VTK_TT*>(source), reinterpret_cast<VTK_TT*>(dest), numTensors));
    }
    dataArray->Delete();
    return tensor;
  }

  if (attrType == XDMF_ATTRIBUTE_TYPE_VECTOR && numComponents == 2)
  {
    // convert 2D vectors to 3-tuple vectors with 0.0 in the z component
    vtkDataArray* vector3D = dataArray->NewInstance();
    vtkIdType numVectors = dataArray->GetNumberOfTuples();
    vector3D->SetNumberOfComponents(3);
    vector3D->SetNumberOfTuples(numVectors);

    // Add 0.0 to third component of vector
    auto inputRange = vtk::DataArrayTupleRange<2>(dataArray);
    auto outputRange = vtk::DataArrayTupleRange<3>(vector3D);
    for (auto i = 0; i < inputRange.size(); ++i)
    {
      outputRange[i][0] = inputRange[i][0];
      outputRange[i][1] = inputRange[i][1];
      outputRange[i][2] = 0.0;
    }
    dataArray->Delete();
    return vector3D;
  }

  return dataArray;
}

//-----------------------------------------------------------------------------
// Read ghost cell/point information. This is simply loaded info a
// vtkGhostType attribute array.
bool vtkXdmfHeavyData::ReadGhostSets(
  vtkDataSet* dataSet, XdmfGrid* xmfGrid, int* vtkNotUsed(update_extents) /*=0*/)
{
  // int data_dimensionality = this->Domain->GetDataDimensionality(xmfGrid);
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value <= 0)
    {
      // not a ghost-set, simply continue.
      continue;
    }
    XdmfInt32 setCenter = xmfSet->GetSetType();
    vtkIdType numElems = 0;
    vtkDataSetAttributes* dsa = 0;
    unsigned char ghostFlag = 0;
    switch (setCenter)
    {
      case XDMF_SET_TYPE_NODE:
        dsa = dataSet->GetPointData();
        numElems = dataSet->GetNumberOfPoints();
        ghostFlag = vtkDataSetAttributes::DUPLICATEPOINT;
        break;

      case XDMF_SET_TYPE_CELL:
        dsa = dataSet->GetCellData();
        numElems = dataSet->GetNumberOfCells();
        ghostFlag = vtkDataSetAttributes::DUPLICATECELL;
        break;

      default:
        vtkWarningWithObjectMacro(
          this->Reader, "Only ghost-cells and ghost-nodes are currently supported.");
        continue;
    }

    vtkUnsignedCharArray* ghosts =
      vtkArrayDownCast<vtkUnsignedCharArray>(dsa->GetArray(vtkDataSetAttributes::GhostArrayName()));
    if (!ghosts)
    {
      ghosts = vtkUnsignedCharArray::New();
      ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
      ghosts->SetNumberOfComponents(1);
      ghosts->SetNumberOfTuples(numElems);
      ghosts->FillComponent(0, 0);
      dsa->AddArray(ghosts);
      ghosts->Delete();
    }

    unsigned char* ptrGhosts = ghosts->GetPointer(0);

    // Read heavy data. We cannot do anything smart if update_extents or stride
    // is specified here. We have to read the entire set and then prune it.
    xmfSet->Update();

    XdmfArray* xmfIds = xmfSet->GetIds();
    XdmfInt64 numIds = xmfIds->GetNumberOfElements();
    std::vector<XdmfInt64> ids(numIds + 1);
    xmfIds->GetValues(0, ids.data(), numIds);

    // release the heavy data that was read.
    xmfSet->Release();

    for (vtkIdType kk = 0; kk < numIds; kk++)
    {
      if (ids[kk] < 0 || ids[kk] > numElems)
      {
        vtkWarningWithObjectMacro(this->Reader, "No such cell or point exists: " << ids[kk]);
        continue;
      }
      ptrGhosts[ids[kk]] = ghostFlag;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkXdmfHeavyData::ReadSets(
  vtkDataSet* dataSet, XdmfGrid* xmfGrid, int* vtkNotUsed(update_extents) /*=0*/)
{
  unsigned int number_of_sets = 0;
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value != 0)
    {
      // skip ghost-sets.
      continue;
    }
    number_of_sets++;
  }
  if (number_of_sets == 0)
  {
    return nullptr;
  }

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
  mb->SetNumberOfBlocks(1 + number_of_sets);
  mb->SetBlock(0, dataSet);
  mb->GetMetaData(static_cast<unsigned int>(0))->Set(vtkCompositeDataSet::NAME(), "Data");

  unsigned int current_set_index = 1;
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value != 0)
    {
      // skip ghost-sets.
      continue;
    }

    const char* setName = xmfSet->GetName();
    mb->GetMetaData(current_set_index)->Set(vtkCompositeDataSet::NAME(), setName);
    if (!this->Domain->GetSetsSelection()->ArrayIsEnabled(setName))
    {
      continue;
    }

    // Okay now we have an enabled set. Create a new dataset for it
    vtkDataSet* set = 0;

    XdmfInt32 setType = xmfSet->GetSetType();
    switch (setType)
    {
      case XDMF_SET_TYPE_NODE:
        set = this->ExtractPoints(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_CELL:
        set = this->ExtractCells(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_FACE:
        set = this->ExtractFaces(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_EDGE:
        set = this->ExtractEdges(xmfSet, dataSet);
        break;
    }

    if (set)
    {
      mb->SetBlock(current_set_index, set);
      set->Delete();
    }
    current_set_index++;
  }
  return mb;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkXdmfHeavyData::ExtractPoints(XdmfSet* xmfSet, vtkDataSet* dataSet)
{
  // TODO: How to handle structured datasets with update_extents/strides etc.
  // Do they too always produce vtkUniformGrid or do we want to produce
  // structured dataset

  // Read heavy data. We cannot do anything smart if update_extents or stride
  // is specified here. We have to read the entire set and then prune it.
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfInt64 numIds = xmfIds->GetNumberOfElements();
  std::vector<XdmfInt64> ids(numIds + 1);
  xmfIds->GetValues(0, ids.data(), numIds);

  // release heavy data.
  xmfSet->Release();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();
  vtkPoints* outputPoints = vtkPoints::New();
  outputPoints->SetNumberOfPoints(numIds);
  output->SetPoints(outputPoints);
  outputPoints->Delete();

  vtkIdType numInPoints = dataSet->GetNumberOfPoints();
  for (vtkIdType kk = 0; kk < numIds; kk++)
  {
    if (ids[kk] < 0 || ids[kk] > numInPoints)
    {
      vtkWarningWithObjectMacro(this->Reader, "No such cell or point exists: " << ids[kk]);
      continue;
    }
    double point_location[3];
    dataSet->GetPoint(ids[kk], point_location);
    outputPoints->SetPoint(kk, point_location);
  }
  ids.clear(); // done with ids

  // Read node-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_NODE)
    {
      continue;
    }
    vtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetPointData()->AddArray(array);
      array->Delete();
    }
  }

  std::vector<vtkIdType> vtk_cell_ids(numIds);
  std::iota(vtk_cell_ids.begin(), vtk_cell_ids.end(), 0);
  output->InsertNextCell(VTK_POLY_VERTEX, numIds, vtk_cell_ids.data());

  return output;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkXdmfHeavyData::ExtractCells(XdmfSet* xmfSet, vtkDataSet* dataSet)
{
  // TODO: How to handle structured datasets with update_extents/strides etc.
  // Do they too always produce vtkUniformGrid or do we want to produce
  // structured dataset

  // Read heavy data.
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfInt64 numIds = xmfIds->GetNumberOfElements();

  vtkIdTypeArray* ids = vtkIdTypeArray::New();
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numIds);
  xmfIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(0), numIds);

  // release heavy data.
  xmfSet->Release();

  // We directly use vtkExtractSelectedIds for extract cells since the logic to
  // extract cells it no trivial (like extracting points).
  vtkSelectionNode* selNode = vtkSelectionNode::New();
  selNode->SetContentType(vtkSelectionNode::INDICES);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(ids);

  vtkSelection* sel = vtkSelection::New();
  sel->AddNode(selNode);
  selNode->Delete();

  vtkExtractSelectedIds* extractCells = vtkExtractSelectedIds::New();
  extractCells->SetInputData(0, dataSet);
  extractCells->SetInputData(1, sel);
  extractCells->Update();

  vtkDataSet* output = vtkDataSet::SafeDownCast(extractCells->GetOutput()->NewInstance());
  output->CopyStructure(vtkDataSet::SafeDownCast(extractCells->GetOutput()));

  sel->Delete();
  extractCells->Delete();
  ids->Delete();

  // Read cell-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_CELL)
    {
      continue;
    }
    vtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkXdmfHeavyData::ExtractFaces(XdmfSet* xmfSet, vtkDataSet* dataSet)
{
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfArray* xmfCellIds = xmfSet->GetCellIds();

  XdmfInt64 numFaces = xmfIds->GetNumberOfElements();

  // ids is a 2 component array were each tuple is (cell-id, face-id).
  vtkIdTypeArray* ids = vtkIdTypeArray::New();
  ids->SetNumberOfComponents(2);
  ids->SetNumberOfTuples(numFaces);
  xmfCellIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(0), numFaces, 1, 2);
  xmfIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(1), numFaces, 1, 2);

  vtkPolyData* output = vtkPolyData::New();
  vtkCellArray* polys = vtkCellArray::New();
  output->SetPolys(polys);
  polys->Delete();

  vtkPoints* outPoints = vtkPoints::New();
  output->SetPoints(outPoints);
  outPoints->Delete();

  vtkMergePoints* mergePoints = vtkMergePoints::New();
  mergePoints->InitPointInsertion(outPoints, dataSet->GetBounds());

  for (vtkIdType cc = 0; cc < numFaces; cc++)
  {
    vtkIdType cellId = ids->GetValue(cc * 2);
    vtkIdType faceId = ids->GetValue(cc * 2 + 1);
    vtkCell* cell = dataSet->GetCell(cellId);
    if (!cell)
    {
      vtkWarningWithObjectMacro(this->Reader, "Invalid cellId: " << cellId);
      continue;
    }
    vtkCell* face = cell->GetFace(faceId);
    if (!face)
    {
      vtkWarningWithObjectMacro(this->Reader, "Invalid faceId " << faceId << " on cell " << cellId);
      continue;
    }

    // Now insert this face a new cell in the output dataset.
    vtkIdType numPoints = face->GetNumberOfPoints();
    vtkPoints* facePoints = face->GetPoints();
    std::vector<vtkIdType> outputPts(numPoints + 1);
    for (vtkIdType kk = 0; kk < numPoints; kk++)
    {
      mergePoints->InsertUniquePoint(facePoints->GetPoint(kk), outputPts[kk]);
    }
    polys->InsertNextCell(numPoints, outputPts.data());
  }

  ids->Delete();
  xmfSet->Release();
  mergePoints->Delete();

  // Read face-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_FACE)
    {
      continue;
    }
    vtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkXdmfHeavyData::ExtractEdges(XdmfSet* xmfSet, vtkDataSet* dataSet)
{
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfArray* xmfCellIds = xmfSet->GetCellIds();
  XdmfArray* xmfFaceIds = xmfSet->GetFaceIds();

  XdmfInt64 numEdges = xmfIds->GetNumberOfElements();

  // ids is a 3 component array were each tuple is (cell-id, face-id, edge-id).
  vtkIdTypeArray* ids = vtkIdTypeArray::New();
  ids->SetNumberOfComponents(3);
  ids->SetNumberOfTuples(numEdges);
  xmfCellIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(0), numEdges, 1, 3);
  xmfFaceIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(1), numEdges, 1, 3);
  xmfIds->GetValues(0, (vtkXdmfIdType*)ids->GetPointer(2), numEdges, 1, 3);

  vtkPolyData* output = vtkPolyData::New();
  vtkCellArray* lines = vtkCellArray::New();
  output->SetLines(lines);
  lines->Delete();

  vtkPoints* outPoints = vtkPoints::New();
  output->SetPoints(outPoints);
  outPoints->Delete();

  vtkMergePoints* mergePoints = vtkMergePoints::New();
  mergePoints->InitPointInsertion(outPoints, dataSet->GetBounds());

  for (vtkIdType cc = 0; cc < numEdges; cc++)
  {
    vtkIdType cellId = ids->GetValue(cc * 3);
    vtkIdType faceId = ids->GetValue(cc * 3 + 1);
    vtkIdType edgeId = ids->GetValue(cc * 3 + 2);
    vtkCell* cell = dataSet->GetCell(cellId);
    if (!cell)
    {
      vtkWarningWithObjectMacro(this->Reader, "Invalid cellId: " << cellId);
      continue;
    }
    vtkCell* face = cell->GetFace(faceId);
    if (!face)
    {
      vtkWarningWithObjectMacro(this->Reader, "Invalid faceId " << faceId << " on cell " << cellId);
      continue;
    }
    vtkCell* edge = cell->GetEdge(edgeId);
    if (!edge)
    {
      vtkWarningWithObjectMacro(this->Reader,
        "Invalid edgeId " << edgeId << " on face " << faceId << " on cell " << cellId);
      continue;
    }

    // Now insert this edge as a new cell in the output dataset.
    vtkIdType numPoints = edge->GetNumberOfPoints();
    vtkPoints* edgePoints = edge->GetPoints();
    std::vector<vtkIdType> outputPts(numPoints + 1);
    for (vtkIdType kk = 0; kk < numPoints; kk++)
    {
      mergePoints->InsertUniquePoint(edgePoints->GetPoint(kk), outputPts[kk]);
    }
    lines->InsertNextCell(numPoints, outputPts.data());
  }

  ids->Delete();
  xmfSet->Release();
  mergePoints->Delete();

  // Read edge-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_EDGE)
    {
      continue;
    }
    vtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}
