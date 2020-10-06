/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_0_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGeometryFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPentagonalPrism.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkTetra.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridCellIterator.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <memory>

vtkStandardNewMacro(vtkGeometryFilter);
vtkCxxSetObjectMacro(vtkGeometryFilter, Locator, vtkIncrementalPointLocator);

//------------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkGeometryFilter::vtkGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_ID_MAX;

  this->Extent[0] = -VTK_DOUBLE_MAX;
  this->Extent[1] = VTK_DOUBLE_MAX;
  this->Extent[2] = -VTK_DOUBLE_MAX;
  this->Extent[3] = VTK_DOUBLE_MAX;
  this->Extent[4] = -VTK_DOUBLE_MAX;
  this->Extent[5] = VTK_DOUBLE_MAX;

  this->PointClipping = false;
  this->CellClipping = false;
  this->ExtentClipping = false;

  this->Merging = true;
  this->Locator = nullptr;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->FastMode = false;
  this->Degree = 4;

  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIdsName = nullptr;
  this->OriginalPointIdsName = nullptr;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);

  // Compatibility with vtkDataSetSurfaceFilter
  this->NonlinearSubdivisionLevel = 1;

  // Enable delegation to an internal vtkDataSetSurfaceFilter.
  this->Delegation = true;
}

//------------------------------------------------------------------------------
vtkGeometryFilter::~vtkGeometryFilter()
{
  this->SetLocator(nullptr);
}

//------------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//------------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGeometryFilter::SetExtent(double extent[6])
{
  int i;

  if (extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
    extent[2] != this->Extent[2] || extent[3] != this->Extent[3] || extent[4] != this->Extent[4] ||
    extent[5] != this->Extent[5])
  {
    this->Modified();
    for (i = 0; i < 3; i++)
    {
      if (extent[2 * i + 1] < extent[2 * i])
      {
        extent[2 * i + 1] = extent[2 * i];
      }
      this->Extent[2 * i] = extent[2 * i];
      this->Extent[2 * i + 1] = extent[2 * i + 1];
    }
  }
}

//------------------------------------------------------------------------------
void vtkGeometryFilter::SetOutputPointsPrecision(int precision)
{
  if (this->OutputPointsPrecision != precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//------------------------------------------------------------------------------
// Excluded faces are defined here.
struct vtkExcludedFaces
{
  vtkStaticCellLinksTemplate<vtkIdType>* Links;
  vtkExcludedFaces()
    : Links(nullptr)
  {
  }
  ~vtkExcludedFaces() { delete this->Links; }
};

//----------------------------------------------------------------------------
int vtkGeometryFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* excInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numPts == 0 || numCells == 0)
  {
    return 1;
  }

  // Check to see if excluded faces have been provided, and is so prepare the data
  // for use.
  vtkExcludedFaces exc; // Will delete exc->Links when goes out of scope
  if (excInfo)
  {
    vtkPolyData* excFaces = vtkPolyData::SafeDownCast(excInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkCellArray* excPolys = excFaces->GetPolys();
    if (excPolys->GetNumberOfCells() > 0)
    {
      exc.Links = new vtkStaticCellLinksTemplate<vtkIdType>;
      exc.Links->ThreadedBuildLinks(numPts, excPolys->GetNumberOfCells(), excPolys);
    }
  }

  // Prepare to delegate based on dataset type and characteristics.
  int dataDim = 0;
  switch (input->GetDataObjectType())
  {
    case VTK_POLY_DATA:
    {
      return this->PolyDataExecute(input, output, &exc);
    }
    case VTK_UNSTRUCTURED_GRID:
    case VTK_UNSTRUCTURED_GRID_BASE:
    {
      return this->UnstructuredGridExecute(input, output, nullptr, &exc);
    }

    // Structured dataset types
    case VTK_RECTILINEAR_GRID:
      dataDim = vtkRectilinearGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_STRUCTURED_GRID:
      dataDim = vtkStructuredGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_UNIFORM_GRID:
      dataDim = vtkUniformGrid::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_STRUCTURED_POINTS:
      dataDim = vtkStructuredPoints::SafeDownCast(input)->GetDataDimension();
      break;
    case VTK_IMAGE_DATA:
      dataDim = vtkImageData::SafeDownCast(input)->GetDataDimension();
      break;
  }

  // Delegate to the faster structured processing if possible. It simplifies
  // things if we only consider 3D structured datasets. Otherwise the
  // general DataSetExecute will handle it just fine.
  if (dataDim == 3)
  {
    return this->StructuredExecute(input, output, inInfo, &exc);
  }

  // Use the general case
  return this->DataSetExecute(input, output, &exc);
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. This method is now deprecated.
void vtkGeometryFilter::CreateDefaultLocator() {}

//------------------------------------------------------------------------------
void vtkGeometryFilter::SetExcludedFacesData(vtkPolyData* input)
{
  this->Superclass::SetInputData(1, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
void vtkGeometryFilter::SetExcludedFacesConnection(vtkAlgorithmOutput* algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
// Reutrn the input data or filter.
vtkPolyData* vtkGeometryFilter::GetExcludedFaces()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");

  os << indent << "Fast Mode: " << (this->FastMode ? "On\n" : "Off\n");
  os << indent << "Degree: " << this->Degree << "\n";

  os << indent << "PieceInvariant: " << this->GetPieceInvariant() << endl;
  os << indent << "PassThroughCellIds: " << (this->GetPassThroughCellIds() ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->GetPassThroughPointIds() ? "On\n" : "Off\n");

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;

  os << indent << "NonlinearSubdivisionLevel: " << this->GetNonlinearSubdivisionLevel() << endl;
}

//------------------------------------------------------------------------------
// Acceleration methods and classes for unstructured grid geometry extraction.
namespace // anonymous
{
// Make things a little more expressive
typedef std::vector<vtkIdType> IdListType;

// This class accumulates cell array-related information. Also marks points
// as used if a point map is provided.
struct CellArrayType
{
  vtkIdType* PointMap;
  IdListType Cells;
  IdListType OrigCellIds;
  vtkIdType* ConnPtr;
  vtkIdType* OffsetsPtr;
  vtkStaticCellLinksTemplate<vtkIdType>* ExcFaces;

  CellArrayType()
    : PointMap(nullptr)
    , ConnPtr(nullptr)
    , OffsetsPtr(nullptr)
    , ExcFaces(nullptr)
  {
  }

  void SetPointMap(vtkIdType* ptMap) { this->PointMap = ptMap; }
  void SetExcludedFaces(vtkStaticCellLinksTemplate<vtkIdType>* exc) { this->ExcFaces = exc; }
  vtkIdType GetNumberOfCells() { return static_cast<vtkIdType>(this->OrigCellIds.size()); }
  vtkIdType GetNumberOfConnEntries() { return static_cast<vtkIdType>(this->Cells.size()); }

  void InsertNextCell(vtkIdType npts, const vtkIdType* pts, vtkIdType cellId)
  {
    // Only insert the face cell if it's not excluded
    if (this->ExcFaces && this->ExcFaces->MatchesCell(npts, pts))
    {
      return;
    }

    // Okay insert the boundary face cell
    Cells.emplace_back(npts);
    if (!this->PointMap)
    {
      for (auto i = 0; i < npts; ++i)
      {
        Cells.emplace_back(pts[i]);
      }
    }
    else
    {
      for (auto i = 0; i < npts; ++i)
      {
        Cells.emplace_back(pts[i]);
        this->PointMap[pts[i]] = 1;
      }
    }
    OrigCellIds.emplace_back(cellId);
  }
};

//--------------------------------------------------------------------------
// Functor/worklet interfaces VTK -> SMPTools threading. This class enables
// compositing the output threads into a final VTK output. The actual work
// is performed by by subclasses of ExtractCellBoundaries which implement
// their own operator() method (i.e., the subclasses specialize
// to a particular dataset type).
struct LocalDataType
{
  // Later on (in Reduce()), a thread id is assigned to the thread.
  int ThreadId;

  // If point merging is specified, then a non-null point map is provided.
  vtkIdType* PointMap;

  // These collect the boundary entities from geometry extraction. Note also
  // that these implicitly keep track of the number of cells inserted.
  CellArrayType Verts;
  CellArrayType Lines;
  CellArrayType Polys;
  CellArrayType Strips;

  // Later (in the Reduce() method) build an offset structure to support
  // threaded compositing of output geometric entities.
  vtkIdType VertsConnOffset;  // this thread's offset into the output vert connectivity
  vtkIdType VertsOffset;      // this thread's offset into the output offsets
  vtkIdType LinesConnOffset;  // this thread's offset into the output line connectivity
  vtkIdType LinesOffset;      // offset into the output line cells
  vtkIdType PolysConnOffset;  // this thread's offset into the output poly connectivity
  vtkIdType PolysOffset;      // offset into the output poly cells
  vtkIdType StripsConnOffset; // this thread's offset into the output strip connectivity
  vtkIdType StripsOffset;     // offset into the output triangle strip cells

  // These are scratch arrays to avoid repeated allocations
  vtkSmartPointer<vtkGenericCell> Cell;
  vtkSmartPointer<vtkIdList> CellIds;
  vtkSmartPointer<vtkIdList> IPts;
  vtkSmartPointer<vtkIdList> ICellIds;
  vtkSmartPointer<vtkPoints> Coords;

  LocalDataType()
  {
    this->PointMap = nullptr;
    this->Cell.TakeReference(vtkGenericCell::New());
    this->CellIds.TakeReference(vtkIdList::New());
    this->IPts.TakeReference(vtkIdList::New());
    this->ICellIds.TakeReference(vtkIdList::New());
    this->Coords.TakeReference(vtkPoints::New());
  }

  void SetPointMap(vtkIdType* ptMap)
  {
    this->PointMap = ptMap;
    this->Verts.SetPointMap(ptMap);
    this->Lines.SetPointMap(ptMap);
    this->Polys.SetPointMap(ptMap);
    this->Strips.SetPointMap(ptMap);
  }

  void SetExcludedFaces(vtkStaticCellLinksTemplate<vtkIdType>* exc)
  {
    this->Verts.SetExcludedFaces(exc);
    this->Lines.SetExcludedFaces(exc);
    this->Polys.SetExcludedFaces(exc);
    this->Strips.SetExcludedFaces(exc);
  }
};
typedef vtkSMPThreadLocal<LocalDataType>::iterator ThreadIterType;
typedef std::vector<ThreadIterType> ThreadOutputType;

//--------------------------------------------------------------------------
// Given a cell and a bunch of supporting objects (to support computing and
// minimize allocation/deallocation), extract boundary features from the cell.
// This method works with arbitrary datasets.
void ExtractDSCellGeometry(
  vtkDataSet* input, vtkIdType cellId, const char* cellVis, LocalDataType* localData)
{
  static const int pixelConvert[4] = { 0, 1, 3, 2 };
  vtkGenericCell* cell = localData->Cell;
  input->GetCell(cellId, cell);
  int cellType = cell->GetCellType();

  if (cellType != VTK_EMPTY_CELL)
  {
    CellArrayType& verts = localData->Verts;
    CellArrayType& lines = localData->Lines;
    CellArrayType& polys = localData->Polys;
    CellArrayType& strips = localData->Strips;
    vtkIdList* cellIds = localData->CellIds.Get();
    vtkIdList* ptIds = localData->IPts.Get();
    ptIds->SetNumberOfIds(4);

    int cellDim = cell->GetCellDimension();
    vtkIdType npts = cell->PointIds->GetNumberOfIds();
    vtkIdType* pts = cell->PointIds->GetPointer(0);

    switch (cellDim)
    {
      // create new points and then cell
      case 0:
        verts.InsertNextCell(npts, pts, cellId);
        break;

      case 1:
        lines.InsertNextCell(npts, pts, cellId);
        break;

      case 2:
        if (cellType == VTK_TRIANGLE_STRIP)
        {
          strips.InsertNextCell(npts, pts, cellId);
        }
        else if (cellType == VTK_PIXEL)
        {
          ptIds->SetId(0, pts[pixelConvert[0]]);
          ptIds->SetId(1, pts[pixelConvert[1]]);
          ptIds->SetId(2, pts[pixelConvert[2]]);
          ptIds->SetId(3, pts[pixelConvert[3]]);
          polys.InsertNextCell(npts, ptIds->GetPointer(0), cellId);
        }
        else
        {
          polys.InsertNextCell(npts, pts, cellId);
        }
        break;

      case 3:
        int numFaces = cell->GetNumberOfFaces();
        for (auto j = 0; j < numFaces; j++)
        {
          vtkCell* face = cell->GetFace(j);
          input->GetCellNeighbors(cellId, face->PointIds, cellIds);
          if (cellIds->GetNumberOfIds() <= 0 || (cellVis && !cellVis[cellIds->GetId(0)]))
          {
            vtkIdType numFacePts = face->GetNumberOfPoints();
            polys.InsertNextCell(numFacePts, face->PointIds->GetPointer(0), cellId);
          }
        }
        break;
    } // switch
  }   // non-empty cell
} // extract dataset geometry

//--------------------------------------------------------------------------
// Given a cell and a bunch of supporting objects (to support computing and
// minimize allocation/deallocation), extract boundary features from the cell.
// This method works with 3D structured data.
void ExtractStructuredCellGeometry(vtkDataSet* input, vtkIdType cellId, int cellType, vtkIdType,
  const vtkIdType* pts, const char* cellVis, LocalDataType* localData)
{
  CellArrayType& polys = localData->Polys;
  vtkIdList* cellIds = localData->CellIds.Get();
  vtkIdList* ptIds = localData->IPts.Get();
  ptIds->SetNumberOfIds(4);

  int faceId, numFacePts;
  const vtkIdType* faceVerts;
  bool insertFace;
  static const int pixelConvert[4] = { 0, 1, 3, 2 };

  switch (cellType)
  {
    case VTK_VOXEL:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkVoxel::GetFaceArray(faceId);
        ptIds->SetId(0, pts[faceVerts[pixelConvert[0]]]);
        ptIds->SetId(1, pts[faceVerts[pixelConvert[1]]]);
        ptIds->SetId(2, pts[faceVerts[pixelConvert[2]]]);
        ptIds->SetId(3, pts[faceVerts[pixelConvert[3]]]);
        input->GetCellNeighbors(cellId, ptIds, cellIds);
        insertFace = (cellIds->GetNumberOfIds() <= 0 || (cellVis && !cellVis[cellIds->GetId(0)]));
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds->GetPointer(0), cellId);
        }
      }
      break;

    case VTK_HEXAHEDRON:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkHexahedron::GetFaceArray(faceId);
        ptIds->SetId(0, pts[faceVerts[0]]);
        ptIds->SetId(1, pts[faceVerts[1]]);
        ptIds->SetId(2, pts[faceVerts[2]]);
        ptIds->SetId(3, pts[faceVerts[3]]);
        input->GetCellNeighbors(cellId, ptIds, cellIds);
        insertFace = (cellIds->GetNumberOfIds() <= 0 || (cellVis && !cellVis[cellIds->GetId(0)]));
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds->GetPointer(0), cellId);
        }
      }
      break;

  } // switch
} // ExtractStructuredCellGeometry()

//--------------------------------------------------------------------------
// Given a cell and a bunch of supporting objects (to support computing and
// minimize allocation/deallocation), extract boundary features from the cell.
// This method works with unstructured grids.
void ExtractCellGeometry(vtkUnstructuredGrid* input, vtkIdType cellId, int cellType, vtkIdType npts,
  const vtkIdType* pts, const char* cellVis, vtkUnstructuredGridCellIterator* cellIter,
  LocalDataType* localData)
{
  CellArrayType& verts = localData->Verts;
  CellArrayType& lines = localData->Lines;
  CellArrayType& polys = localData->Polys;
  CellArrayType& strips = localData->Strips;
  vtkGenericCell* cell = localData->Cell.Get();
  vtkIdList* cellIds = localData->CellIds.Get();

  int faceId, numFacePts;
  const int MAX_FACE_POINTS = 32;
  vtkIdType ptIds[MAX_FACE_POINTS]; // cell face point ids
  const vtkIdType* faceVerts;
  bool insertFace;
  static const int pixelConvert[4] = { 0, 1, 3, 2 };

  switch (cellType)
  {
    case VTK_EMPTY_CELL:
      break;

    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      verts.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_LINE:
    case VTK_POLY_LINE:
      lines.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_POLYGON:
      polys.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_TRIANGLE_STRIP:
      strips.InsertNextCell(npts, pts, cellId);
      break;

    case VTK_PIXEL:
      // pixelConvert (in the following loop) is an int[4]. GCC 5.1.1
      // warns about pixelConvert[4] being uninitialized due to loop
      // unrolling -- forcibly restricting npts <= 4 prevents this warning.
      ptIds[0] = pts[pixelConvert[0]];
      ptIds[1] = pts[pixelConvert[1]];
      ptIds[2] = pts[pixelConvert[2]];
      ptIds[3] = pts[pixelConvert[3]];
      polys.InsertNextCell(npts, ptIds, cellId);
      break;

    case VTK_TETRA:
      numFacePts = 3;
      for (faceId = 0; faceId < 4; faceId++)
      {
        faceVerts = vtkTetra::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_VOXEL:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkVoxel::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[pixelConvert[0]]];
        ptIds[1] = pts[faceVerts[pixelConvert[1]]];
        ptIds[2] = pts[faceVerts[pixelConvert[2]]];
        ptIds[3] = pts[faceVerts[pixelConvert[3]]];
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_HEXAHEDRON:
      numFacePts = 4;
      for (faceId = 0; faceId < 6; faceId++)
      {
        faceVerts = vtkHexahedron::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_WEDGE:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkWedge::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        numFacePts = 3;
        if (faceVerts[3] >= 0)
        {
          ptIds[3] = pts[faceVerts[3]];
          numFacePts = 4;
        }
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_PYRAMID:
      for (faceId = 0; faceId < 5; faceId++)
      {
        faceVerts = vtkPyramid::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        numFacePts = 3;
        if (faceVerts[3] >= 0)
        {
          ptIds[3] = pts[faceVerts[3]];
          numFacePts = 4;
        }
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_HEXAGONAL_PRISM:
      for (faceId = 0; faceId < 8; faceId++)
      {
        faceVerts = vtkHexagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        numFacePts = 4;
        if (faceVerts[4] >= 0)
        {
          ptIds[4] = pts[faceVerts[4]];
          ptIds[5] = pts[faceVerts[5]];
          numFacePts = 6;
        }
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    case VTK_PENTAGONAL_PRISM:
      for (faceId = 0; faceId < 7; faceId++)
      {
        faceVerts = vtkPentagonalPrism::GetFaceArray(faceId);
        ptIds[0] = pts[faceVerts[0]];
        ptIds[1] = pts[faceVerts[1]];
        ptIds[2] = pts[faceVerts[2]];
        ptIds[3] = pts[faceVerts[3]];
        numFacePts = 4;
        if (faceVerts[4] >= 0)
        {
          ptIds[4] = pts[faceVerts[4]];
          numFacePts = 5;
        }
        if (!cellVis) // most common, fastpath: geometry not cropped
        {
          insertFace = input->IsCellBoundary(cellId, numFacePts, ptIds);
        }
        else // slower path, geometry cropped via point id, cell id, and/or extent
        {
          input->GetCellNeighbors(cellId, numFacePts, ptIds, cellIds);
          insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
        }
        if (insertFace)
        {
          polys.InsertNextCell(numFacePts, ptIds, cellId);
        }
      }
      break;

    default:
      // Other types of 3D linear cells handled by vtkGeometryFilter. Exactly what
      // is a linear cell is defined by vtkCellTypes::IsLinear().
      cellIter->GetCell(cell);
      if (cell->GetCellDimension() == 3)
      {
        int numFaces = cell->GetNumberOfFaces();
        for (auto j = 0; j < numFaces; j++)
        {
          vtkCell* face = cell->GetFace(j);
          numFacePts = face->PointIds->GetNumberOfIds();
          if (!cellVis) // most common, fastpath: geometry not cropped
          {
            insertFace = input->IsCellBoundary(cellId, numFacePts, face->PointIds->GetPointer(0));
          }
          else // slower path, geometry cropped via point id, cell id, and/or extent
          {
            input->GetCellNeighbors(cellId, numFacePts, face->PointIds->GetPointer(0), cellIds);
            insertFace = (cellIds->GetNumberOfIds() <= 0 || !cellVis[cellIds->GetId(0)]);
          }
          if (insertFace)
          {
            polys.InsertNextCell(numFacePts, face->PointIds->GetPointer(0), cellId);
          }
        } // for all cell faces
      }   // if 3D
      else
      {
        vtkLog(ERROR, "Unknown cell type.");
      }
  } // switch
} // ExtractCellGeometry()

// Base class to extract boundary entities. Derived by all dataset extraction
// types -- the operator() method needs to be implemented by subclasses.
struct ExtractCellBoundaries
{
  // If point merging is specified, then a point map is created.
  vtkIdType* PointMap;

  // Cell visibility and cell ghost levels
  const char* CellVis;
  const unsigned char* CellGhosts;

  // These are the final composited output cell arrays
  vtkCellArray* Verts;       // output verts
  vtkIdType* VertsConnPtr;   // output connectivity array
  vtkIdType* VertsOffsetPtr; // output offsets array

  vtkCellArray* Lines; // output lines
  vtkIdType* LinesConnPtr;
  vtkIdType* LinesOffsetPtr;

  vtkCellArray* Polys; // output polys
  vtkIdType* PolysConnPtr;
  vtkIdType* PolysOffsetPtr;

  vtkCellArray* Strips; // output triangle strips
  vtkIdType* StripsConnPtr;
  vtkIdType* StripsOffsetPtr;

  // Thread-related output data
  vtkSMPThreadLocal<LocalDataType> LocalData;
  vtkIdType VertsCellIdOffset;
  vtkIdType LinesCellIdOffset;
  vtkIdType PolysCellIdOffset;
  vtkIdType StripsCellIdOffset;
  vtkIdType NumPts;
  vtkIdType NumCells;
  ExtractCellBoundaries* Extract;
  vtkStaticCellLinksTemplate<vtkIdType>* ExcFaces;
  ThreadOutputType* Threads;

  ExtractCellBoundaries(const char* cellVis, const unsigned char* ghosts, vtkCellArray* verts,
    vtkCellArray* lines, vtkCellArray* polys, vtkCellArray* strips, vtkExcludedFaces* exc,
    ThreadOutputType* threads)
    : PointMap(nullptr)
    , CellVis(cellVis)
    , CellGhosts(ghosts)
    , Verts(verts)
    , Lines(lines)
    , Polys(polys)
    , Strips(strips)
    , Threads(threads)
  {
    this->ExcFaces = (exc == nullptr ? nullptr : exc->Links);
    this->VertsConnPtr = this->VertsOffsetPtr = nullptr;
    this->LinesConnPtr = this->LinesOffsetPtr = nullptr;
    this->PolysConnPtr = this->PolysOffsetPtr = nullptr;
    this->StripsConnPtr = this->StripsOffsetPtr = nullptr;
  }

  virtual ~ExtractCellBoundaries() { delete[] this->PointMap; }

  // If point merging is needed, create the point map (map from old points
  // to new points).
  void CreatePointMap(vtkIdType numPts)
  {
    this->PointMap = new vtkIdType[numPts];
    std::fill_n(this->PointMap, numPts, (-1));
  }

  // Helper function supporting Reduce() to allocate and construct output cell arrays.
  // Also keep local information to facilitate compositing.
  void AllocateCellArray(vtkIdType connSize, vtkIdType numCells, vtkCellArray* ca,
    vtkIdType*& connPtr, vtkIdType*& offsetPtr)
  {
    vtkNew<vtkIdTypeArray> outConn;
    connPtr = outConn->WritePointer(0, connSize);
    vtkNew<vtkIdTypeArray> outOffsets;
    offsetPtr = outOffsets->WritePointer(0, numCells + 1);
    offsetPtr[numCells] = connSize;
    ca->SetData(outOffsets, outConn);
  }

  // Initialize thread data
  void Initialize()
  {
    // Make sure cells have been built
    auto& localData = this->LocalData.Local();
    localData.SetPointMap(this->PointMap);
    localData.SetExcludedFaces(this->ExcFaces);
  }

  // operator() implemented by dataset-specific subclasses

  // Composite local thread data; i.e., rather than linearly appending data from each
  // thread into the filter's output, this performs a parallel append.
  void Reduce()
  {
    // Determine offsets to partition work and perform memory allocations.
    vtkIdType numCells, numConnEntries;
    vtkIdType vertsNumPts = 0, vertsNumCells = 0;
    vtkIdType linesNumPts = 0, linesNumCells = 0;
    vtkIdType polysNumPts = 0, polysNumCells = 0;
    vtkIdType stripsNumPts = 0, stripsNumCells = 0;
    int threadId = 0;

    // Loop over the local data generated by each thread. Setup the
    // offsets and such to insert into the output cell arrays.
    auto tItr = this->LocalData.begin();
    auto tEnd = this->LocalData.end();
    for (; tItr != tEnd; ++tItr)
    {
      tItr->ThreadId = threadId++;
      this->Threads->emplace_back(tItr); // need pointers to local thread data

      tItr->VertsConnOffset = vertsNumPts;
      tItr->VertsOffset = vertsNumCells;
      numCells = tItr->Verts.GetNumberOfCells();
      numConnEntries = tItr->Verts.GetNumberOfConnEntries() - numCells;
      vertsNumCells += numCells;
      vertsNumPts += numConnEntries;

      tItr->LinesConnOffset = linesNumPts;
      tItr->LinesOffset = linesNumCells;
      numCells = tItr->Lines.GetNumberOfCells();
      numConnEntries = tItr->Lines.GetNumberOfConnEntries() - numCells;
      linesNumCells += numCells;
      linesNumPts += numConnEntries;

      tItr->PolysConnOffset = polysNumPts;
      tItr->PolysOffset = polysNumCells;
      numCells = tItr->Polys.GetNumberOfCells();
      numConnEntries = tItr->Polys.GetNumberOfConnEntries() - numCells;
      polysNumCells += numCells;
      polysNumPts += numConnEntries;

      tItr->StripsConnOffset = stripsNumPts;
      tItr->StripsOffset = stripsNumCells;
      numCells = tItr->Strips.GetNumberOfCells();
      numConnEntries = tItr->Strips.GetNumberOfConnEntries() - numCells;
      stripsNumCells += numCells;
      stripsNumPts += numConnEntries;
    }
    this->VertsCellIdOffset = 0;
    this->LinesCellIdOffset = vertsNumCells;
    this->PolysCellIdOffset = vertsNumCells + linesNumCells;
    this->StripsCellIdOffset = vertsNumCells + linesNumCells + polysNumCells;
    this->NumCells = vertsNumCells + linesNumCells + polysNumCells + stripsNumCells;
    this->NumPts = vertsNumPts + linesNumPts + polysNumPts + stripsNumPts;

    // Allocate data for the output cell arrays: connectivity and
    // offsets are required to construct a cell array. Later compositing
    // will fill them in.
    if (vertsNumPts > 0)
    {
      this->AllocateCellArray(
        vertsNumPts, vertsNumCells, this->Verts, this->VertsConnPtr, this->VertsOffsetPtr);
    }
    if (linesNumPts > 0)
    {
      this->AllocateCellArray(
        linesNumPts, linesNumCells, this->Lines, this->LinesConnPtr, this->LinesOffsetPtr);
    }
    if (polysNumPts > 0)
    {
      this->AllocateCellArray(
        polysNumPts, polysNumCells, this->Polys, this->PolysConnPtr, this->PolysOffsetPtr);
    }
    if (stripsNumPts > 0)
    {
      this->AllocateCellArray(
        stripsNumPts, stripsNumCells, this->Strips, this->StripsConnPtr, this->StripsOffsetPtr);
    }
  }
};

// Extract unstructured grid boundary by visiting each cell and examining
// cell features.
struct ExtractUG : public ExtractCellBoundaries
{
  // The unstructured grid to process
  vtkUnstructuredGrid* Grid;
  // Each thread has its own cell iterator.
  vtkSMPThreadLocal<vtkSmartPointer<vtkUnstructuredGridCellIterator>> CellIter;

  ExtractUG(vtkUnstructuredGrid* grid, const char* cellVis, const unsigned char* ghosts,
    bool merging, vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys,
    vtkCellArray* strips, vtkExcludedFaces* exc, ThreadOutputType* t)
    : ExtractCellBoundaries(cellVis, ghosts, verts, lines, polys, strips, exc, t)
    , Grid(grid)
    , CellIter(nullptr)
  {
    if (merging)
    {
      this->CreatePointMap(grid->GetNumberOfPoints());
    }
  }

  // Initialize thread data
  void Initialize()
  {
    this->ExtractCellBoundaries::Initialize();
    this->CellIter.Local().TakeReference(
      static_cast<vtkUnstructuredGridCellIterator*>(this->Grid->NewCellIterator()));
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& cellIter = this->CellIter.Local();

    for (cellIter->GoToCell(cellId); cellId < endCellId; ++cellId, cellIter->GoToNextCell())
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // If the cell is visible process it
      if (!this->CellVis || this->CellVis[cellId])
      {
        int type = cellIter->GetCellType();
        vtkIdList* pointIdList = cellIter->GetPointIds();
        vtkIdType npts = pointIdList->GetNumberOfIds();
        vtkIdType* pts = pointIdList->GetPointer(0);

        ExtractCellGeometry(
          this->Grid, cellId, type, npts, pts, this->CellVis, cellIter, &localData);
      } // if cell visible
    }   // for all cells in this batch
  }     // operator()

  // Composite local thread data
  void Reduce() { this->ExtractCellBoundaries::Reduce(); }
};

// Extract unstructured grid boundary by visiting cells connected to points
// with low point degree.
struct FastExtractUG : public ExtractCellBoundaries
{
  // The unstructured grid to process
  vtkUnstructuredGrid* Grid;
  // Each thread has its own cell iterator.
  vtkSMPThreadLocal<vtkSmartPointer<vtkUnstructuredGridCellIterator>> CellIter;

  // Specialized to the fast unstructured grid extraction process.
  vtkAbstractCellLinks* Links;
  vtkIdType Degree;
  std::vector<unsigned char> CellSelection;

  FastExtractUG(vtkUnstructuredGrid* grid, const char* cellVis, const unsigned char* ghosts,
    bool merging, vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys,
    vtkCellArray* strips, vtkIdType degree, vtkAbstractCellLinks* links, vtkExcludedFaces* exc,
    ThreadOutputType* t)
    : ExtractCellBoundaries(cellVis, ghosts, verts, lines, polys, strips, exc, t)
    , Grid(grid)
    , CellIter(nullptr)
    , Links(links)
    , Degree(degree)
  {
    vtkIdType minMaxDegree[2];
    minMaxDegree[0] = 0;
    minMaxDegree[1] = this->Degree + 1;
    this->CellSelection.resize(grid->GetNumberOfCells());
    this->Links->SelectCells(minMaxDegree, CellSelection.data());
    if (merging)
    {
      this->CreatePointMap(grid->GetNumberOfPoints());
    }
  }

  // Initialize thread data
  void Initialize()
  {
    this->ExtractCellBoundaries::Initialize();
    this->CellIter.Local().TakeReference(
      static_cast<vtkUnstructuredGridCellIterator*>(this->Grid->NewCellIterator()));
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& cellIter = this->CellIter.Local();

    for (cellIter->GoToCell(cellId); cellId < endCellId; ++cellId, cellIter->GoToNextCell())
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // If the cell is visible process it
      if (this->CellSelection[cellId] != 0 && (!this->CellVis || this->CellVis[cellId]))
      {
        int type = cellIter->GetCellType();
        vtkIdList* pointIdList = cellIter->GetPointIds();
        vtkIdType npts = pointIdList->GetNumberOfIds();
        vtkIdType* pts = pointIdList->GetPointer(0);

        ExtractCellGeometry(
          this->Grid, cellId, type, npts, pts, this->CellVis, cellIter, &localData);

      } // if cell visible and selected via fast mode (vertex degree)
    }   // for all cells in this batch
  }     // operator()

  // Composite local thread data
  void Reduce() { this->ExtractCellBoundaries::Reduce(); }
}; // FastExtractUG

// Extract structured 3D grid boundary by visiting each cell and examining
// cell features.
struct ExtractStructured : public ExtractCellBoundaries
{
  vtkDataSet* Input; // Input data
  vtkIdType* Extent; // Data extent
  int Dims[3];       // Grid dimensions

  ExtractStructured(vtkDataSet* ds, vtkIdType ext[6], const char* cellVis,
    const unsigned char* ghosts, bool merging, vtkCellArray* polys, vtkExcludedFaces* exc,
    ThreadOutputType* t)
    : ExtractCellBoundaries(cellVis, ghosts, nullptr, nullptr, polys, nullptr, exc, t)
    , Input(ds)
    , Extent(ext)
  {
    this->Dims[0] = this->Extent[1] - this->Extent[0] + 1;
    this->Dims[1] = this->Extent[3] - this->Extent[2] + 1;
    this->Dims[2] = this->Extent[5] - this->Extent[4] + 1;
    if (merging)
    {
      this->CreatePointMap(this->Dims[0] * this->Dims[1] * this->Dims[2]);
    }
  }

  // Determine whether to process the structured cell at location ijk[3]
  // and with cellId given for face extraction.
  bool ProcessCell(vtkIdType cellId, int ijk[3])
  {
    // Are we on the boundary of the structured dataset? Then the cell will
    // certainly produce a boundary face.
    if (ijk[0] == 0 || ijk[0] == (this->Dims[0] - 2) || ijk[1] == 0 ||
      ijk[1] == (this->Dims[1] - 2) || ijk[2] == 0 || ijk[2] == (this->Dims[2] - 2))
    {
      return true;
    }

    // If a cell visibility array is provided, then check neighbors.  If a
    // neighbor is not visible, then this cell will produce a boundary
    // face.  Note that since we've already checked the boundary cells (in
    // the if statement above) we don't need to worry about indexing beyond
    // the end of the cellVis array.
    if (this->CellVis)
    {
      int yInc = this->Dims[0] - 1;
      int zInc = (this->Dims[0] - 1) * (this->Dims[1] - 1);
      const char* cellVis = this->CellVis;
      if (!cellVis[cellId - 1] || !cellVis[cellId + 1] || !cellVis[cellId - yInc] ||
        !cellVis[cellId + yInc] || !cellVis[cellId - zInc] || !cellVis[cellId + zInc])
      {
        return true;
      }
    }

    return false;
  }

  // Initialize thread data
  void Initialize() { this->ExtractCellBoundaries::Initialize(); }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();

    for (; cellId < endCellId; ++cellId)
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // If the cell is visible process it. This is far from optimized but simple.
      if (!this->CellVis || this->CellVis[cellId])
      {
        // Get the ijk to see if this cell is on the boundary of the structured data.
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoords(cellId, this->Dims, ijk);
        if (this->ProcessCell(cellId, ijk))
        { // on boundary
          vtkGenericCell* cell = localData.Cell;
          this->Input->GetCell(cellId, cell);
          int cellType = cell->GetCellType();
          vtkIdType npts = cell->PointIds->GetNumberOfIds();
          vtkIdType* pts = cell->PointIds->GetPointer(0);
          ExtractStructuredCellGeometry(
            this->Input, cellId, cellType, npts, pts, this->CellVis, &localData);
        }
      } // if cell visible
    }   // for all cells in this batch
  }     // operator()

  // Composite local thread data
  void Reduce() { this->ExtractCellBoundaries::Reduce(); }
};

// Extract the boundaries of a general vtkDataSet by visiting each cell and
// examining cell features. This is slower than specialized types and should be
// avoided if possible.
struct ExtractDS : public ExtractCellBoundaries
{
  // The unstructured grid to process
  vtkDataSet* DataSet;

  ExtractDS(vtkDataSet* ds, const char* cellVis, const unsigned char* ghosts, vtkCellArray* verts,
    vtkCellArray* lines, vtkCellArray* polys, vtkCellArray* strips, vtkExcludedFaces* exc,
    ThreadOutputType* t)
    : ExtractCellBoundaries(cellVis, ghosts, verts, lines, polys, strips, exc, t)
    , DataSet(ds)
  {
    // Point merging is always required since points are not explicitly
    // represented and cannot be passed through to the output.
    this->CreatePointMap(ds->GetNumberOfPoints());
  }

  // Initialize thread data
  void Initialize()
  {
    this->ExtractCellBoundaries::Initialize();
    // Make sure any internal initialization methods which may not be thread
    // safe are built.
    this->DataSet->GetCell(0);
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();

    for (; cellId < endCellId; ++cellId)
    {
      // Handle ghost cells here.  Another option was used cellVis array.
      if (this->CellGhosts && this->CellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
      { // Do not create surfaces in outer ghost cells.
        continue;
      }

      // If the cell is visible process it
      if (!this->CellVis || this->CellVis[cellId])
      {
        ExtractDSCellGeometry(this->DataSet, cellId, this->CellVis, &localData);
      } // if cell visible

    } // for all cells in this batch
  }   // operator()

  // Composite local thread data
  void Reduce() { this->ExtractCellBoundaries::Reduce(); }
};

// Helper class to record original point and cell ids. This is for copying
// cell data, and also to produce output arrays indicating where output
// cells originated from (typically used in picking).
struct IdRecorder
{
  vtkSmartPointer<vtkIdTypeArray> Ids;

  IdRecorder(
    vtkTypeBool passThru, const char* name, vtkDataSetAttributes* attrD, vtkIdType allocSize)
  {
    if (passThru)
    {
      this->Ids.TakeReference(vtkIdTypeArray::New());
      this->Ids->SetName(name);
      this->Ids->SetNumberOfComponents(1);
      this->Ids->Allocate(allocSize);
      attrD->AddArray(this->Ids.Get());
    }
  }
  IdRecorder(vtkTypeBool passThru, const char* name, vtkDataSetAttributes* attrD)
  {
    if (passThru)
    {
      this->Ids.TakeReference(vtkIdTypeArray::New());
      this->Ids->SetName(name);
      this->Ids->SetNumberOfComponents(1);
      attrD->AddArray(this->Ids.Get());
    }
    else
    {
      this->Ids = nullptr;
    }
  }
  void Insert(vtkIdType destId, vtkIdType origId)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->InsertValue(destId, origId);
    }
  }
  vtkIdType* GetPointer() { return this->Ids->GetPointer(0); }
  vtkTypeBool PassThru() { return (this->Ids.Get() == nullptr ? false : true); }
  void Allocate(vtkIdType num)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->Allocate(num);
    }
  }
  void SetNumberOfValues(vtkIdType num)
  {
    if (this->Ids.Get() != nullptr)
    {
      this->Ids->SetNumberOfValues(num);
    }
  }
}; // id recorder

// Generate point map for explicit point representations.
template <typename TIP, typename TOP>
struct GenerateExpPoints
{
  TIP* InPts;
  TOP* OutPts;
  vtkIdType* PointMap;
  ArrayList* PtArrays;

  GenerateExpPoints(TIP* inPts, TOP* outPts, vtkIdType* ptMap, ArrayList* ptArrays)
    : InPts(inPts)
    , OutPts(outPts)
    , PointMap(ptMap)
    , PtArrays(ptArrays)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto inPts = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    vtkIdType mapId;

    for (; ptId < endPtId; ++ptId)
    {
      if ((mapId = this->PointMap[ptId]) >= 0)
      {
        auto xIn = inPts[ptId];
        auto xOut = outPts[mapId];
        xOut[0] = xIn[0];
        xOut[1] = xIn[1];
        xOut[2] = xIn[2];
        this->PtArrays->Copy(ptId, mapId);
      }
    }
  }
};

// Generate point map for implicit point representations.
template <typename TOP>
struct GenerateImpPoints
{
  vtkDataSet* InPts;
  TOP* OutPts;
  vtkIdType* PointMap;
  ArrayList* PtArrays;

  GenerateImpPoints(vtkDataSet* inPts, TOP* outPts, vtkIdType* ptMap, ArrayList* ptArrays)
    : InPts(inPts)
    , OutPts(outPts)
    , PointMap(ptMap)
    , PtArrays(ptArrays)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    double xIn[3];
    vtkIdType mapId;

    for (; ptId < endPtId; ++ptId)
    {
      if ((mapId = this->PointMap[ptId]) >= 0)
      {
        this->InPts->GetPoint(ptId, xIn);
        auto xOut = outPts[mapId];
        xOut[0] = xIn[0];
        xOut[1] = xIn[1];
        xOut[2] = xIn[2];
        this->PtArrays->Copy(ptId, mapId);
      }
    }
  }
};

// Base class for point generation workers.
struct GeneratePtsWorker
{
  vtkIdType NumOutputPoints;

  GeneratePtsWorker()
    : NumOutputPoints(0)
  {
  }

  // Create the final point map. This could be threaded (prefix_sum) but
  // performance gains are minimal.
  vtkIdType* GeneratePointMap(vtkIdType numInputPts, ExtractCellBoundaries* extract)
  {
    // The PointMap has been marked as to which points are being used.
    // This needs to be updated to indicate the output point ids.
    vtkIdType* ptMap = extract->PointMap;
    for (auto ptId = 0; ptId < numInputPts; ++ptId)
    {
      if (ptMap[ptId] == 1)
      {
        ptMap[ptId] = this->NumOutputPoints++;
      }
    }
    return ptMap;
  }
};

// Dispatch to explicit, templated point types
struct ExpPtsWorker : public GeneratePtsWorker
{
  template <typename TIP, typename TOP>
  void operator()(TIP* inPts, TOP* outPts, vtkIdType numInputPts, vtkPointData* inPD,
    vtkPointData* outPD, ExtractCellBoundaries* extract)
  {
    // Finalize the point map
    vtkIdType* ptMap = this->GeneratePointMap(numInputPts, extract);

    // Now generate all of the points and point attribute data
    ArrayList ptArrays;
    outPD->CopyAllocate(inPD, this->NumOutputPoints);
    ptArrays.AddArrays(this->NumOutputPoints, inPD, outPD, 0.0, false);

    outPts->SetNumberOfTuples(this->NumOutputPoints);
    GenerateExpPoints<TIP, TOP> genPts(inPts, outPts, ptMap, &ptArrays);
    vtkSMPTools::For(0, numInputPts, genPts);
  }
};

// Dispatch to implicit input points, explicit output points
struct ImpPtsWorker : public GeneratePtsWorker
{
  template <typename TOP>
  void operator()(TOP* outPts, vtkDataSet* inPts, vtkIdType numInputPts, vtkPointData* inPD,
    vtkPointData* outPD, ExtractCellBoundaries* extract)
  {
    // Finalize the point map
    vtkIdType* ptMap = this->GeneratePointMap(numInputPts, extract);

    // Now generate all of the points and point attribute data
    ArrayList ptArrays;
    outPD->CopyAllocate(inPD, this->NumOutputPoints);
    ptArrays.AddArrays(this->NumOutputPoints, inPD, outPD, 0.0, false);

    outPts->SetNumberOfTuples(this->NumOutputPoints);
    GenerateImpPoints<TOP> genPts(inPts, outPts, ptMap, &ptArrays);
    vtkSMPTools::For(0, numInputPts, genPts);
  }
};

// Composite threads to produce output cell topology
struct CompositeCells
{
  const vtkIdType* PointMap;
  ArrayList* CellArrays;
  ExtractCellBoundaries* Extractor;
  ThreadOutputType* Threads;

  CompositeCells(vtkIdType* ptMap, ArrayList* cellArrays, ExtractCellBoundaries* extract,
    ThreadOutputType* threads)
    : PointMap(ptMap)
    , CellArrays(cellArrays)
    , Extractor(extract)
    , Threads(threads)
  {
  }

  void CompositeCellArray(CellArrayType* cat, vtkIdType connOffset, vtkIdType offset,
    vtkIdType cellIdOffset, vtkIdType* connPtr, vtkIdType* offsetPtr)
  {
    vtkIdType* cells = cat->Cells.data();
    vtkIdType numCells = cat->GetNumberOfCells();
    connPtr += connOffset;
    offsetPtr += offset;
    vtkIdType offsetVal = connOffset;
    vtkIdType globalCellId = cellIdOffset + offset;

    // If not merging points, we reuse input points and so do not need to
    // produce new points nor point data.
    if (!this->PointMap)
    {
      for (auto cellId = 0; cellId < numCells; ++cellId)
      {
        *offsetPtr++ = offsetVal;
        vtkIdType npts = *cells++;
        for (auto i = 0; i < npts; ++i)
        {
          *connPtr++ = *cells++;
        }
        offsetVal += npts;
        this->CellArrays->Copy(cat->OrigCellIds[cellId], globalCellId++);
      }
    }
    else // Merging - i.e., using a point map
    {
      for (auto cellId = 0; cellId < numCells; ++cellId)
      {
        *offsetPtr++ = offsetVal;
        vtkIdType npts = *cells++;
        for (auto i = 0; i < npts; ++i)
        {
          *connPtr++ = this->PointMap[*cells++];
        }
        offsetVal += npts;
        this->CellArrays->Copy(cat->OrigCellIds[cellId], globalCellId++);
      }
    }
  }

  void operator()(vtkIdType thread, vtkIdType threadEnd)
  {
    ExtractCellBoundaries* extract = this->Extractor;

    for (; thread < threadEnd; ++thread)
    {
      ThreadIterType tItr = (*this->Threads)[thread];

      if (extract->VertsConnPtr)
      {
        this->CompositeCellArray(&tItr->Verts, tItr->VertsConnOffset, tItr->VertsOffset,
          extract->VertsCellIdOffset, extract->VertsConnPtr, extract->VertsOffsetPtr);
      }
      if (extract->LinesConnPtr)
      {
        this->CompositeCellArray(&tItr->Lines, tItr->LinesConnOffset, tItr->LinesOffset,
          extract->LinesCellIdOffset, extract->LinesConnPtr, extract->LinesOffsetPtr);
      }
      if (extract->PolysConnPtr)
      {
        this->CompositeCellArray(&tItr->Polys, tItr->PolysConnOffset, tItr->PolysOffset,
          extract->PolysCellIdOffset, extract->PolysConnPtr, extract->PolysOffsetPtr);
      }
      if (extract->StripsConnPtr)
      {
        this->CompositeCellArray(&tItr->Strips, tItr->StripsConnOffset, tItr->StripsOffset,
          extract->StripsCellIdOffset, extract->StripsConnPtr, extract->StripsOffsetPtr);
      }
    }
  }
}; // CompositeCells

// Composite threads to produce originating cell ids
struct CompositeCellIds
{
  ExtractCellBoundaries* Extractor;
  ThreadOutputType* Threads;
  vtkIdType* OrigIds;

  CompositeCellIds(ExtractCellBoundaries* extract, ThreadOutputType* threads, vtkIdType* origIds)
    : Extractor(extract)
    , Threads(threads)
    , OrigIds(origIds)
  {
  }

  void CompositeIds(CellArrayType* cat, vtkIdType offset, vtkIdType cellIdOffset)
  {
    vtkIdType numCells = cat->GetNumberOfCells();
    vtkIdType globalCellId = cellIdOffset + offset;

    for (auto cellId = 0; cellId < numCells; ++cellId)
    {
      this->OrigIds[globalCellId++] = cat->OrigCellIds[cellId];
    }
  }

  void operator()(vtkIdType thread, vtkIdType threadEnd)
  {
    ExtractCellBoundaries* extract = this->Extractor;

    for (; thread < threadEnd; ++thread)
    {
      ThreadIterType tItr = (*this->Threads)[thread];

      if (extract->VertsConnPtr)
      {
        this->CompositeIds(&tItr->Verts, tItr->VertsOffset, extract->VertsCellIdOffset);
      }
      if (extract->LinesConnPtr)
      {
        this->CompositeIds(&tItr->Lines, tItr->LinesOffset, extract->LinesCellIdOffset);
      }
      if (extract->PolysConnPtr)
      {
        this->CompositeIds(&tItr->Polys, tItr->PolysOffset, extract->PolysCellIdOffset);
      }
      if (extract->StripsConnPtr)
      {
        this->CompositeIds(&tItr->Strips, tItr->StripsOffset, extract->StripsCellIdOffset);
      }
    }
  }
}; // CompositeCellIds

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkGeometryFilter::PolyDataExecute(vtkDataSet* dataSetInput, vtkPolyData* output)
{
  return this->PolyDataExecute(dataSetInput, output, nullptr);
}

//------------------------------------------------------------------------------
// This is currently not threaded. Usually polydata extraction is only used to
// setup originating cell or point ids - this part is threaded.
int vtkGeometryFilter::PolyDataExecute(
  vtkDataSet* dataSetInput, vtkPolyData* output, vtkExcludedFaces* exc)
{
  vtkPolyData* input = static_cast<vtkPolyData*>(dataSetInput);
  vtkIdType cellId;
  int i;
  int allVisible;
  vtkIdType npts;
  const vtkIdType* pts;
  vtkPoints* p = input->GetPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  vtkIdType newCellId, ptId;
  int visible, type;
  double x[3];
  unsigned char* cellGhosts = nullptr;
  vtkStaticCellLinksTemplate<vtkIdType>* links = (exc == nullptr ? nullptr : exc->Links);

  vtkDebugMacro(<< "Executing geometry filter for poly data input");

  vtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
  }
  else
  {
    allVisible = 0;
  }

  IdRecorder origCellIds(
    this->PassThroughCellIds, this->GetOriginalCellIdsName(), output->GetCellData());
  IdRecorder origPointIds(
    this->PassThroughPointIds, this->GetOriginalPointIdsName(), output->GetPointData());

  // vtkPolyData points are not culled
  if (origPointIds.PassThru())
  {
    origPointIds.SetNumberOfValues(numPts);
    vtkIdType* origPointIdsPtr = origPointIds.GetPointer();
    vtkSMPTools::For(0, numPts, [&origPointIdsPtr](vtkIdType pId, vtkIdType endPId) {
      for (; pId < endPId; ++pId)
      {
        origPointIdsPtr[pId] = pId;
      }
    });
  }

  // Special case when data is just passed through
  if (allVisible && links == nullptr)
  {
    output->CopyStructure(input);
    outputPD->PassData(pd);
    outputCD->PassData(cd);

    if (origCellIds.PassThru())
    {
      origCellIds.SetNumberOfValues(numCells);
      vtkIdType* origCellIdsPtr = origCellIds.GetPointer();
      vtkSMPTools::For(0, numCells, [&origCellIdsPtr](vtkIdType cId, vtkIdType endCId) {
        for (; cId < endCId; ++cId)
        {
          origCellIdsPtr[cId] = cId;
        }
      });
    }

    return 1;
  }

  // Okay slower path, clipping by cells and/or point ids, or excluding
  // faces. Cells may be culled. Always pass point data (points are not
  // culled).
  output->SetPoints(p);
  outputPD->PassData(pd);

  // Allocate
  //
  origCellIds.Allocate(numCells);
  origPointIds.Allocate(numPts);

  output->AllocateEstimate(numCells, 1);
  outputCD->CopyAllocate(cd, numCells, numCells / 2);
  input->BuildCells(); // needed for GetCellPoints()

  vtkIdType progressInterval = numCells / 20 + 1;
  for (cellId = 0; cellId < numCells; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      vtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    input->GetCellPoints(cellId, npts, pts);

    visible = 1;
    if (!allVisible)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        visible = 0;
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          ptId = pts[i];
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            visible = 0;
            break;
          }
        }
      }
    }

    // now if visible extract geometry - i.e., cells may be culled
    if ((allVisible || visible) && (!links || !links->MatchesCell(npts, pts)))
    {
      type = input->GetCellType(cellId);
      newCellId = output->InsertNextCell(type, npts, pts);
      outputCD->CopyData(cd, cellId, newCellId);
      origCellIds.Insert(cellId, newCellId);
    } // if visible
  }   // for all cells

  // Update ourselves and release memory
  //
  output->Squeeze();

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  return 1;
}

namespace
{
struct CharacterizeGrid
{
  vtkUnstructuredGrid* Grid;
  unsigned char* Types;
  unsigned char IsLinear;
  vtkSMPThreadLocal<unsigned char> LocalIsLinear;

  CharacterizeGrid(vtkUnstructuredGrid* grid, vtkUnsignedCharArray* types)
    : Grid(grid)
  {
    this->Types = types->GetPointer(0);
  }

  void Initialize() { this->LocalIsLinear.Local() = 1; }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    if (!this->LocalIsLinear.Local())
    {
      return;
    }
    // Check against linear cell types
    for (; cellId < endCellId; ++cellId)
    {
      if (!vtkCellTypes::IsLinear(this->Types[cellId]))
      {
        this->LocalIsLinear.Local() = 0;
        return;
      }
    }
  }

  void Reduce()
  {
    this->IsLinear = 1;
    auto tItr = this->LocalIsLinear.begin();
    auto tEnd = this->LocalIsLinear.end();
    for (; tItr != tEnd; ++tItr)
    {
      if (*tItr == 0)
      {
        this->IsLinear = 0;
        return;
      }
    }
  }
};

// Threaded creation to generate array of originating point ids.
void PassPointIds(const char* name, vtkIdType numInputPts, vtkIdType numOutputPts, vtkIdType* ptMap,
  vtkPointData* outPD)
{
  vtkNew<vtkIdTypeArray> origPtIds;
  origPtIds->SetName(name);
  origPtIds->SetNumberOfComponents(1);
  origPtIds->SetNumberOfTuples(numOutputPts);
  outPD->AddArray(origPtIds);
  vtkIdType* origIds = origPtIds->GetPointer(0);

  // Now threaded populate the array
  vtkSMPTools::For(0, numInputPts, [&origIds, &ptMap](vtkIdType ptId, vtkIdType endPtId) {
    for (; ptId < endPtId; ++ptId)
    {
      if (ptMap[ptId] >= 0)
      {
        origIds[ptMap[ptId]] = ptId;
      }
    }
  });
}

// Threaded compositing of originating cell ids.
void PassCellIds(
  const char* name, ExtractCellBoundaries* extract, ThreadOutputType* threads, vtkCellData* outCD)
{
  vtkIdType numOutputCells = extract->NumCells;
  vtkNew<vtkIdTypeArray> origCellIds;
  origCellIds->SetName(name);
  origCellIds->SetNumberOfComponents(1);
  origCellIds->SetNumberOfTuples(numOutputCells);
  outCD->AddArray(origCellIds);
  vtkIdType* origIds = origCellIds->GetPointer(0);

  // Now populate the original cell ids
  CompositeCellIds compIds(extract, threads, origIds);
  vtkSMPTools::For(0, static_cast<vtkIdType>(threads->size()), compIds);
}

} // anonymous

//----------------------------------------------------------------------------
vtkGeometryFilterHelper* vtkGeometryFilterHelper::CharacterizeUnstructuredGrid(
  vtkUnstructuredGrid* input)
{
  vtkGeometryFilterHelper* info = new vtkGeometryFilterHelper;

  // Check to see if the data actually has nonlinear cells.  Handling
  // nonlinear cells requires delegation to the appropriate filter.
  vtkIdType numCells = input->GetNumberOfCells();
  CharacterizeGrid characterize(input, input->GetCellTypesArray());
  vtkSMPTools::For(0, numCells, characterize);

  info->IsLinear = characterize.IsLinear;

  return info;
}

//----------------------------------------------------------------------------
void vtkGeometryFilterHelper::CopyFilterParams(vtkGeometryFilter* gf, vtkDataSetSurfaceFilter* dssf)
{
  // Helper method to copy key parameters from this filter to an instance of
  // vtkDataSetSurfaceFilter. This is for delegation.
  dssf->SetPieceInvariant(gf->GetPieceInvariant());
  dssf->SetPassThroughCellIds(gf->GetPassThroughCellIds());
  dssf->SetPassThroughPointIds(gf->GetPassThroughPointIds());
  dssf->SetOriginalCellIdsName(gf->GetOriginalCellIdsName());
  dssf->SetOriginalPointIdsName(gf->GetOriginalPointIdsName());
  dssf->SetNonlinearSubdivisionLevel(gf->GetNonlinearSubdivisionLevel());
}

//----------------------------------------------------------------------------
void vtkGeometryFilterHelper::CopyFilterParams(vtkDataSetSurfaceFilter* dssf, vtkGeometryFilter* gf)
{
  // Helper method to copy key parameters from an instance of
  // vtkDataSetSurfaceFilter to vtkGeometryFilter. This is for delegation.
  gf->SetPieceInvariant(dssf->GetPieceInvariant());
  gf->SetPassThroughCellIds(dssf->GetPassThroughCellIds());
  gf->SetPassThroughPointIds(dssf->GetPassThroughPointIds());
  gf->SetOriginalCellIdsName(dssf->GetOriginalCellIdsName());
  gf->SetOriginalPointIdsName(dssf->GetOriginalPointIdsName());
  gf->SetNonlinearSubdivisionLevel(dssf->GetNonlinearSubdivisionLevel());
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::UnstructuredGridExecute(vtkDataSet* dataSetInput, vtkPolyData* output)
{
  return this->UnstructuredGridExecute(dataSetInput, output, nullptr, nullptr);
}

//----------------------------------------------------------------------------
int vtkGeometryFilter::UnstructuredGridExecute(vtkDataSet* dataSetInput, vtkPolyData* output,
  vtkGeometryFilterHelper* info, vtkExcludedFaces* exc)
{
  vtkUnstructuredGrid* input = static_cast<vtkUnstructuredGrid*>(dataSetInput);
  vtkCellArray* connectivity = input->GetCells();
  if (connectivity == nullptr)
  {
    vtkDebugMacro(<< "Nothing to extract");
    return 0;
  }

  // If no info, then compute information about the unstructured grid.
  // Depending on the outcome, we may process the data ourselves, or send over
  // to the faster vtkGeometryFilter.
  bool mayDelegate = (info == nullptr && this->Delegation);
  if (info == nullptr)
  {
    info = vtkGeometryFilterHelper::CharacterizeUnstructuredGrid(input);
  }

  // Nonlinear cells are handled by vtkDataSetSurfaceFilter
  // non-linear cells using sub-division.
  if (!info->IsLinear && mayDelegate)
  {
    vtkNew<vtkDataSetSurfaceFilter> dssf;
    vtkGeometryFilterHelper::CopyFilterParams(this, dssf.Get());
    dssf->UnstructuredGridExecute(dataSetInput, output, info);
    delete info;
    return 1;
  }

  auto cellIter = vtk::TakeSmartPointer(input->NewCellIterator());
  vtkIdType cellId;
  vtkIdType npts = 0;
  vtkIdList* pointIdList;
  const vtkIdType* pts = nullptr;
  vtkPoints* inPts = input->GetPoints();
  vtkIdType numInputPts = input->GetNumberOfPoints(), numOutputPts;
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  std::unique_ptr<char> uCellVis; // reference count to prevent leakage
  char* cellVis;
  unsigned char* cellGhosts = nullptr;

  vtkDebugMacro(<< "Executing geometry filter for unstructured grid input");

  vtkDataArray* temp = nullptr;
  if (inCD)
  {
    temp = inCD->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  // Determine nature of what we have to do
  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    cellVis = nullptr;
  }
  else
  {
    uCellVis = std::unique_ptr<char>(new char[numCells]);
    cellVis = uCellVis.get();
  }

  outCD->CopyGlobalIdsOn();

  // Loop over the cells determining what's visible. This could be threaded
  // if necessary - for now it's not used very often so serial.
  if (cellVis)
  {
    double x[3];
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      cellId = cellIter->GetCellId();
      pointIdList = cellIter->GetPointIds();
      npts = pointIdList->GetNumberOfIds();
      pts = pointIdList->GetPointer(0);
      cellVis[cellId] = 1;
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        for (int i = 0; i < npts; i++)
        {
          inPts->GetPoint(pts[i], x);
          if ((this->PointClipping &&
                (pts[i] < this->PointMinimum || pts[i] > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          } // point/extent clipping
        }   // for each point
      }     // if point clipping needs checking
    }       // for all cells
  }         // if not all visible

  // Prepare to generate the output. The cell arrays are of course the output vertex,
  // line, polygon, and triangle strip output. The four IdListType's capture the
  // generating cell ids (used later to copy cell attributes).
  vtkNew<vtkPoints> outPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  if (!this->Merging) // no merging, just use input points
  {
    output->SetPoints(inPts);
    outPD->PassData(inPD);
  }
  else
  {
    output->SetPoints(outPts);
  }

  vtkNew<vtkCellArray> verts;
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> polys;
  vtkNew<vtkCellArray> strips;

  output->SetVerts(verts);
  output->SetLines(lines);
  output->SetPolys(polys);
  output->SetStrips(strips);

  // Make sure links are built since link building is not thread safe
  input->BuildLinks();

  // Threaded visit of each cell to extract boundary features. Each thread gathers
  // output which is then composited into the final vtkPolyData.
  // Keep track of each thread's output, we'll need this later for compositing.
  ThreadOutputType threads;

  // Perform the threaded boundary cell extraction. This performs some
  // initial reduction and allocation of the output. It also computes offets
  // and sizes for allocation and writing of data.
  ExtractCellBoundaries* extract;
  if (this->FastMode)
  {
    FastExtractUG* ext = new FastExtractUG(input, cellVis, cellGhosts, this->Merging, verts, lines,
      polys, strips, this->Degree, input->GetCellLinks(), exc, &threads);
    vtkSMPTools::For(0, numCells, *ext);
    extract = ext;
  }
  else // the usual path
  {
    ExtractUG* ext = new ExtractUG(
      input, cellVis, cellGhosts, this->Merging, verts, lines, polys, strips, exc, &threads);

    vtkSMPTools::For(0, numCells, *ext);
    extract = ext;
  }
  numCells = extract->NumCells;

  // If merging points, then it's necessary to allocate the points array,
  // configure the point map, and generate the new points. Here we are using
  // an explicit point dispatch (i.e., the point representation is explicitly
  // represented by a data array as we are processing an unstructured grid).
  vtkIdType* ptMap = extract->PointMap;
  if (this->Merging)
  {
    using vtkArrayDispatch::Reals;
    using ExpPtsDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
    ExpPtsWorker compWorker;
    if (!ExpPtsDispatch::Execute(
          inPts->GetData(), outPts->GetData(), compWorker, numInputPts, inPD, outPD, extract))
    { // Fallback to slowpath for other point types
      compWorker(inPts->GetData(), outPts->GetData(), numInputPts, inPD, outPD, extract);
    }
    numOutputPts = compWorker.NumOutputPoints;

    // Generate originating point ids if requested and merging is
    // on. (Generating these originating point ids only makes sense if the
    // points are merged.)
    if (this->PassThroughPointIds)
    {
      PassPointIds(this->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
    }
  }

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

  CompositeCells compCells(ptMap, &cellArrays, extract, &threads);
  vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

  // Generate originating cell ids if requested.
  if (this->PassThroughCellIds)
  {
    PassCellIds(this->GetOriginalCellIdsName(), extract, &threads, outCD);
  }

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Clean up and get out
  delete extract;
  return 1;
}

//------------------------------------------------------------------------------
// Process various types of structured datasets.
int vtkGeometryFilter::StructuredExecute(vtkDataSet* input, vtkPolyData* output, vtkInformation*)
{
  return this->StructuredExecute(input, output, nullptr, nullptr);
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::StructuredExecute(
  vtkDataSet* input, vtkPolyData* output, vtkInformation*, vtkExcludedFaces* exc)
{
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType i, cellId, ptId;
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  // Setup processing
  bool mergePts = true; // implicit point representations require merging
  vtkIdType ext[6];
  int* tmpext;
  vtkPoints* inPts = nullptr;
  switch (input->GetDataObjectType())
  {
    case VTK_STRUCTURED_GRID:
    {
      vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(input);
      tmpext = grid->GetExtent();
      inPts = vtkStructuredGrid::SafeDownCast(input)->GetPoints();
      mergePts = this->Merging; // may not be required for explicit
      break;
    }
    case VTK_RECTILINEAR_GRID:
    {
      vtkRectilinearGrid* grid = vtkRectilinearGrid::SafeDownCast(input);
      tmpext = grid->GetExtent();
      break;
    }
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
    {
      vtkImageData* image = vtkImageData::SafeDownCast(input);
      tmpext = image->GetExtent();
      break;
    }
    default:
      return 0;
  }

  // Update the extent
  ext[0] = tmpext[0];
  ext[1] = tmpext[1];
  ext[2] = tmpext[2];
  ext[3] = tmpext[3];
  ext[4] = tmpext[4];
  ext[5] = tmpext[5];

  // Ghost cells and visibility if necessary
  std::unique_ptr<char> uCellVis; // reference count to prevent leakage
  char* cellVis;
  unsigned char* cellGhosts = nullptr;
  vtkDataArray* temp = inCD->GetArray(vtkDataSetAttributes::GhostArrayName());
  if (inCD)
  {
    temp = inCD->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  // Determine nature of what we have to do
  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    cellVis = nullptr;
  }
  else
  {
    uCellVis = std::unique_ptr<char>(new char[numCells]);
    cellVis = uCellVis.get();
  }

  // Mark cells as being visible or not
  //
  if (cellVis)
  {
    vtkNew<vtkGenericCell> cell;
    vtkIdList* ptIds;
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        double x[3];
        input->GetCell(cellId, cell);
        ptIds = cell->GetPointIds();
        vtkIdType ncells = ptIds->GetNumberOfIds();
        for (i = 0; i < ncells; i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }                // for all points defining the cell
        if (i >= ncells) // if no points are clipped
        {
          cellVis[cellId] = 1;
        }
      } // check cell clipping first, and then point clipping if necessary
    }   // for all cells
  }

  // We can now extract the boundary topology. This works for all structured
  // types. Here we are only dealing with 3D structured datasets. The 2D cells
  // are handled as a general dataset.
  vtkNew<vtkCellArray> polys;
  output->SetPolys(polys);
  ThreadOutputType threads;

  ExtractStructured extStr(input, ext, cellVis, cellGhosts, mergePts, polys, exc, &threads);
  vtkSMPTools::For(0, numCells, extStr);
  numCells = extStr.NumCells;

  // Generate the output points
  vtkIdType numInputPts = input->GetNumberOfPoints(), numOutputPts;
  vtkNew<vtkPoints> outPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION && inPts != nullptr)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  if (!mergePts && inPts != nullptr) // no merging, just use input points
  {
    output->SetPoints(inPts);
    outPD->PassData(inPD);
  }
  else
  {
    output->SetPoints(outPts);
  }

  if (mergePts && inPts != nullptr) // are these explicit points with merging on?
  {
    using vtkArrayDispatch::Reals;
    using ExpPtsDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
    ExpPtsWorker compWorker;
    if (!ExpPtsDispatch::Execute(
          inPts->GetData(), outPts->GetData(), compWorker, numInputPts, inPD, outPD, &extStr))
    { // Fallback to slowpath for other point types
      compWorker(inPts->GetData(), outPts->GetData(), numInputPts, inPD, outPD, &extStr);
    }
    numOutputPts = compWorker.NumOutputPoints;
  }
  else // implicit point representation
  {
    // Some of these datasets have explicit point representations, we'll generate
    // the geometry (i.e., points) now.
    using vtkArrayDispatch::Reals;
    using ImpPtsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    ImpPtsWorker compWorker;
    if (!ImpPtsDispatch::Execute(
          outPts->GetData(), compWorker, input, numInputPts, inPD, outPD, &extStr))
    { // Fallback to slowpath for other point types
      compWorker(outPts->GetData(), input, numInputPts, inPD, outPD, &extStr);
    }
    numOutputPts = compWorker.NumOutputPoints;
  }

  // Generate originating point ids if requested and merging is
  // on. (Generating these originating point ids only makes sense if the
  // points are merged.)
  vtkIdType* ptMap = extStr.PointMap;
  if (this->PassThroughPointIds && (inPts == nullptr || mergePts))
  {
    PassPointIds(this->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
  }

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

  CompositeCells compCells(ptMap, &cellArrays, &extStr, &threads);
  vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

  // Generate originating cell ids if requested.
  if (this->PassThroughCellIds)
  {
    PassCellIds(this->GetOriginalCellIdsName(), &extStr, &threads, outCD);
  }

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  return 1;
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::DataSetExecute(vtkDataSet* input, vtkPolyData* output)
{
  return this->DataSetExecute(input, output, nullptr);
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::DataSetExecute(vtkDataSet* input, vtkPolyData* output, vtkExcludedFaces* exc)
{
  vtkIdType cellId;
  int i;
  vtkIdType numCells = input->GetNumberOfCells();
  double x[3];
  vtkIdType ptId;
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  std::unique_ptr<char> uCellVis; // reference count to prevent leakage
  char* cellVis;
  unsigned char* cellGhosts = nullptr;

  vtkDebugMacro(<< "Executing geometry filter");

  if (numCells == 0)
  {
    vtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  // Ghost cells if necessary
  vtkDataArray* temp = inCD->GetArray(vtkDataSetAttributes::GhostArrayName());
  if (inCD)
  {
    temp = inCD->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<vtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  // Determine nature of what we have to do
  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    cellVis = nullptr;
  }
  else
  {
    uCellVis = std::unique_ptr<char>(new char[numCells]);
    cellVis = uCellVis.get();
  }

  // Mark cells as being visible or not
  //
  if (cellVis)
  {
    vtkNew<vtkGenericCell> cell;
    vtkIdList* ptIds;
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        input->GetCell(cellId, cell);
        ptIds = cell->GetPointIds();
        for (i = 0; i < ptIds->GetNumberOfIds(); i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
        if (i >= ptIds->GetNumberOfIds())
        {
          cellVis[cellId] = 1;
        }
      }
    }
  }

  // Create new output points. In a dataset, points are assumed to be
  // implicitly represented, so merging must occur,
  vtkNew<vtkPoints> outPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION ||
    this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }
  output->SetPoints(outPts);

  vtkNew<vtkCellArray> verts;
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> polys;
  vtkNew<vtkCellArray> strips;

  output->SetVerts(verts);
  output->SetLines(lines);
  output->SetPolys(polys);
  output->SetStrips(strips);

  outPD->CopyGlobalIdsOn();
  outCD->CopyGlobalIdsOn();

  // The extraction process for vtkDataSet
  ThreadOutputType threads;
  ExtractDS extract(input, cellVis, cellGhosts, verts, lines, polys, strips, exc, &threads);

  vtkSMPTools::For(0, numCells, extract);
  numCells = extract.NumCells;

  // If merging points, then it's necessary to allocate the points
  // array. This will be populated later when the final compositing
  // occurs.
  vtkIdType numInputPts = input->GetNumberOfPoints(), numOutputPts;

  // Generate the new points
  using vtkArrayDispatch::Reals;
  using ImpPtsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  ImpPtsWorker compWorker;
  if (!ImpPtsDispatch::Execute(
        outPts->GetData(), compWorker, input, numInputPts, inPD, outPD, &extract))
  { // Fallback to slowpath for other point types
    compWorker(outPts->GetData(), input, numInputPts, inPD, outPD, &extract);
  }
  numOutputPts = compWorker.NumOutputPoints;

  // Generate originating point ids if requested and merging is
  // on. (Generating these originating point ids only makes sense if the
  // points are merged.)
  vtkIdType* ptMap = extract.PointMap;
  if (this->PassThroughPointIds)
  {
    PassPointIds(this->GetOriginalPointIdsName(), numInputPts, numOutputPts, ptMap, outPD);
  }

  // Finally we can composite the output topology.
  ArrayList cellArrays;
  outCD->CopyAllocate(inCD, numCells);
  cellArrays.AddArrays(numCells, inCD, outCD, 0.0, false);

  CompositeCells compCells(ptMap, &cellArrays, &extract, &threads);
  vtkSMPTools::For(0, static_cast<vtkIdType>(threads.size()), compCells);

  // Generate originating cell ids if requested.
  if (this->PassThroughCellIds)
  {
    PassCellIds(this->GetOriginalCellIdsName(), &extract, &threads, outCD);
  }

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  return 1;
}

//------------------------------------------------------------------------------
int vtkGeometryFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}
