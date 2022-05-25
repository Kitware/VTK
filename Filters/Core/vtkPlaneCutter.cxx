/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaneCutter.h"

#include "vtk3DLinearGridPlaneCutter.h"
#include "vtkAppendDataSets.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkFlyingEdgesPlaneCutter.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMarchingCubesPolygonCases.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataPlaneCutter.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSphereTree.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTransform.h"
#include "vtkUniformGridAMR.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>

vtkObjectFactoryNewMacro(vtkPlaneCutter);
vtkCxxSetObjectMacro(vtkPlaneCutter, Plane, vtkPlane);

//------------------------------------------------------------------------------
namespace // begin anonymous namespace
{
//------------------------------------------------------------------------------
struct vtkLocalDataType
{
  vtkPolyData* Output;
  vtkNonMergingPointLocator* Locator;
  vtkCellData* NewVertsData;
  vtkCellData* NewLinesData;
  vtkCellData* NewPolysData;

  vtkLocalDataType()
    : Output(nullptr)
    , Locator(nullptr)
  {
  }
};

//------------------------------------------------------------------------------
// This handles points of any type. InOutArray is allocated here but should
// be deleted by the invoking code. InOutArray is an unsigned char array to
// simplify bit fiddling later on.
template <typename TPointsArray>
struct InOutPlanePoints
{
  TPointsArray* PointsArray;
  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  double Origin[3];
  double Normal[3];

  InOutPlanePoints(TPointsArray* ptsArray, vtkPlane* plane)
    : PointsArray(ptsArray)
  {
    this->InOutArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->InOutArray->SetNumberOfValues(this->PointsArray->GetNumberOfTuples());
    plane->GetOrigin(this->Origin);
    plane->GetNormal(this->Normal);
  }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    double *n = this->Normal, *o = this->Origin;
    const auto& points = vtk::DataArrayTupleRange<3>(this->PointsArray, beginPtId, endPtId);
    auto inOut = vtk::DataArrayValueRange<1>(this->InOutArray, beginPtId, endPtId);
    auto pointsItr = points.begin();
    auto inOutItr = inOut.begin();
    for (; pointsItr != points.end(); ++pointsItr, ++inOutItr)
    {
      // Access each point
      p[0] = static_cast<double>((*pointsItr)[0]);
      p[1] = static_cast<double>((*pointsItr)[1]);
      p[2] = static_cast<double>((*pointsItr)[2]);

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      eval = vtkPlane::Evaluate(n, o, p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *inOutItr = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
  }

  static vtkSmartPointer<vtkUnsignedCharArray> Execute(TPointsArray* ptsArray, vtkPlane* plane)
  {
    InOutPlanePoints<TPointsArray> inOutPlanePoints(ptsArray, plane);
    vtkSMPTools::For(0, ptsArray->GetNumberOfTuples(), inOutPlanePoints);
    return inOutPlanePoints.InOutArray;
  }
};

//------------------------------------------------------------------------------
// This functor uses thread local storage to create one vtkPolyData per
// thread. Each execution of the functor adds to the vtkPolyData that is
// local to the thread it is running on.
template <typename TPointsArray>
struct CuttingFunctor
{
  vtkDataSet* Input;
  TPointsArray* InPointsArray;
  vtkMultiPieceDataSet* OutputMP;
  vtkPlane* Plane;
  vtkSphereTree* SphereTree;
  const unsigned char* Selected;
  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  unsigned char* InOut;
  int OutputPrecision;

  vtkSMPThreadLocal<vtkDoubleArray*> CellScalars;
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkSMPThreadLocalObject<vtkIdList> CellPointIds;
  vtkSMPThreadLocalObject<vtkPoints> NewPts;
  vtkSMPThreadLocalObject<vtkCellArray> NewVerts;
  vtkSMPThreadLocalObject<vtkCellArray> NewLines;
  vtkSMPThreadLocalObject<vtkCellArray> NewPolys;

  vtkSMPThreadLocal<vtkLocalDataType> LocalData;

  double* Origin;
  double* Normal;
  vtkIdType NumSelected;
  bool Interpolate;
  bool GeneratePolygons;

  CuttingFunctor(vtkDataSet* input, TPointsArray* pointsArray, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate, bool generatePolygons = false)
    : Input(input)
    , InPointsArray(pointsArray)
    , OutputPrecision(outputPrecision)
    , OutputMP(outputMP)
    , Plane(plane)
    , SphereTree(tree)
    , InOutArray(nullptr)
    , Origin(origin)
    , Normal(normal)
    , Interpolate(interpolate)
    , GeneratePolygons(generatePolygons)
  {
  }

  virtual ~CuttingFunctor()
  {
    // Cleanup all allocated temporaries
    for (auto& cellScalars : this->CellScalars)
    {
      cellScalars->Delete();
    }

    for (auto& data : this->LocalData)
    {
      data.Output->Delete();
      data.Locator->Delete();
    }
  }

  void BuildAccelerationStructure()
  {
    // To speed computation, either a sphere tree or fast classification
    // process is used.
    if (this->SphereTree)
    {
      this->Selected = this->SphereTree->SelectPlane(this->Origin, this->Normal, this->NumSelected);
    }
    else
    {
      this->InOutArray = InOutPlanePoints<TPointsArray>::Execute(this->InPointsArray, this->Plane);
      this->InOut = this->InOutArray->GetPointer(0);
    }
  }

  bool IsCellSlicedByPlane(vtkIdType cellId, vtkIdList* ptIds)
  {
    this->Input->GetCellPoints(cellId, ptIds);
    vtkIdType npts = ptIds->GetNumberOfIds();
    vtkIdType* pts = ptIds->GetPointer(0);
    // ArePointsAroundPlane
    unsigned char onOneSideOfPlane = this->InOut[pts[0]];
    for (vtkIdType i = 1; onOneSideOfPlane && i < npts; ++i)
    {
      onOneSideOfPlane &= this->InOut[pts[i]];
    }
    return (!onOneSideOfPlane);
  }

  virtual void Initialize()
  {
    // Initialize thread local object before any processing happens.
    // This gets called once per thread.
    vtkLocalDataType& localData = this->LocalData.Local();

    localData.Output = vtkPolyData::New();
    vtkPolyData* output = localData.Output;

    localData.Locator = vtkNonMergingPointLocator::New();
    vtkPointLocator* locator = localData.Locator;

    vtkIdType numCells = this->Input->GetNumberOfCells();

    int precisionType = (this->OutputPrecision == vtkAlgorithm::DEFAULT_PRECISION
        ? this->InPointsArray->GetDataType()
        : (this->OutputPrecision == vtkAlgorithm::SINGLE_PRECISION ? VTK_FLOAT : VTK_DOUBLE));
    vtkPoints*& newPts = this->NewPts.Local();
    newPts->SetDataType(precisionType);
    output->SetPoints(newPts);

    vtkIdType estimatedSize = static_cast<vtkIdType>(sqrt(static_cast<double>(numCells)));
    estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
    estimatedSize = (estimatedSize < 1024 ? 1024 : estimatedSize);

    newPts->Allocate(estimatedSize, estimatedSize);

    // bounds are not important for non-merging locator
    double bounds[6];
    bounds[0] = bounds[2] = bounds[4] = (VTK_FLOAT_MIN);
    bounds[1] = bounds[3] = bounds[5] = (VTK_FLOAT_MAX);
    locator->InitPointInsertion(newPts, bounds, this->Input->GetNumberOfPoints());

    vtkCellArray*& newVerts = this->NewVerts.Local();
    newVerts->AllocateEstimate(estimatedSize, 1);
    output->SetVerts(newVerts);

    vtkCellArray*& newLines = this->NewLines.Local();
    newLines->AllocateEstimate(estimatedSize, 2);
    output->SetLines(newLines);

    vtkCellArray*& newPolys = this->NewPolys.Local();
    newPolys->AllocateEstimate(estimatedSize, 4);
    output->SetPolys(newPolys);

    vtkDoubleArray*& cellScalars = this->CellScalars.Local();
    cellScalars = vtkDoubleArray::New();
    cellScalars->SetNumberOfComponents(1);
    cellScalars->Allocate(VTK_CELL_SIZE);

    vtkPointData* outPd = output->GetPointData();
    vtkCellData* outCd = output->GetCellData();
    vtkPointData* inPd = this->Input->GetPointData();
    vtkCellData* inCd = this->Input->GetCellData();
    if (this->Interpolate)
    {
      outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
      outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);
    }
  }

  virtual void Reduce()
  {
    this->OutputMP->Initialize();
    this->OutputMP->SetNumberOfPieces(static_cast<unsigned int>(this->LocalData.size()));
    // Create the final multi-piece
    int count = 0;
    for (auto& out : this->LocalData)
    {
      vtkPolyData* output = out.Output;
      this->OutputMP->SetPiece(count++, output);
      output->GetFieldData()->PassData(this->Input->GetFieldData());
    }
  }
};

//------------------------------------------------------------------------------
// Process unstructured grids/polyData
template <class TGrid, typename TPointsArray>
struct UnstructuredDataFunctor : public CuttingFunctor<TPointsArray>
{
  UnstructuredDataFunctor(TGrid* inputGrid, TPointsArray* pointsArray, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate)
    : CuttingFunctor<TPointsArray>(
        inputGrid, pointsArray, outputPrecision, outputMP, plane, tree, origin, normal, interpolate)
  {
    if (auto polyData = vtkPolyData::SafeDownCast(inputGrid))
    {
      // create cells map for vtkPolyData
      if (polyData->NeedToBuildCells())
      {
        polyData->BuildCells();
      }
    }
  }

  ~UnstructuredDataFunctor() override
  {
    if (this->Interpolate)
    {
      for (auto& data : this->LocalData)
      {
        data.NewVertsData->Delete();
        data.NewLinesData->Delete();
        data.NewPolysData->Delete();
      }
    }
  }

  void Initialize() override
  {
    CuttingFunctor<TPointsArray>::Initialize();

    // Initialize specific cell data
    if (this->Interpolate)
    {
      vtkLocalDataType& localData = this->LocalData.Local();
      vtkCellData* inCD = this->Input->GetCellData();
      localData.NewVertsData = vtkCellData::New();
      localData.NewLinesData = vtkCellData::New();
      localData.NewPolysData = vtkCellData::New();
      localData.NewVertsData->CopyAllocate(inCD);
      localData.NewLinesData->CopyAllocate(inCD);
      localData.NewPolysData->CopyAllocate(inCD);
    }
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    // Actual computation.
    // Note the usage of thread local objects. These objects
    // persist for each thread across multiple execution of the
    // functor.
    vtkLocalDataType& localData = this->LocalData.Local();
    vtkPointLocator* loc = localData.Locator;

    vtkGenericCell* cell = this->Cell.Local();
    vtkDoubleArray* cellScalars = this->CellScalars.Local();
    vtkPointData* inPD = this->Input->GetPointData();
    vtkCellData* inCD = this->Input->GetCellData();

    vtkPolyData* output = localData.Output;
    vtkPointData* outPD = nullptr;

    vtkCellArray* newVerts = this->NewVerts.Local();
    vtkCellArray* newLines = this->NewLines.Local();
    vtkCellArray* newPolys = this->NewPolys.Local();

    vtkCellData* newVertsData = nullptr;
    vtkCellData* newLinesData = nullptr;
    vtkCellData* newPolysData = nullptr;
    vtkCellData* tmpOutCD = nullptr;
    if (this->Interpolate)
    {
      outPD = output->GetPointData();
      newVertsData = localData.NewVertsData;
      newLinesData = localData.NewLinesData;
      newPolysData = localData.NewPolysData;
    }

    bool needCell;
    double* s;
    int i, numPts;
    vtkPoints* cellPoints;
    const unsigned char* selected = this->Selected + beginCellId;

    vtkIdList*& cellPointIds = this->CellPointIds.Local();
    // Loop over the cell, processing only the one that are needed
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      needCell = false;
      if (this->SphereTree)
      {
        if (*selected++)
        {
          // only the cell whose bounding sphere intersect with the plane are needed
          needCell = true;
        }
      }
      else
      {
        // without a sphere tree, use the inOutPoints
        needCell = this->IsCellSlicedByPlane(cellId, cellPointIds);
      }
      if (needCell)
      {
        this->Input->GetCell(cellId, cell);
        numPts = cell->GetNumberOfPoints();
        cellScalars->SetNumberOfTuples(numPts);
        s = cellScalars->GetPointer(0);
        cellPoints = cell->GetPoints();
        for (i = 0; i < numPts; i++)
        {
          *s++ = this->Plane->FunctionValue(cellPoints->GetPoint(i));
        }

        tmpOutCD = nullptr;
        if (this->Interpolate)
        {
          // Select correct cell data
          switch (cell->GetCellDimension())
          {
            case (0):
              VTK_FALLTHROUGH;
            case (1):
              tmpOutCD = newVertsData;
              break;
            case (2):
              tmpOutCD = newLinesData;
              break;
            case (3):
              tmpOutCD = newPolysData;
              break;
            default:
              break;
          }
        }
        cell->Contour(
          0.0, cellScalars, loc, newVerts, newLines, newPolys, inPD, outPD, inCD, cellId, tmpOutCD);
      }
    }
  }

  void Reduce() override
  {
    CuttingFunctor<TPointsArray>::Reduce();
    if (this->Interpolate)
    {
      // Add specific cell data
      for (auto& out : this->LocalData)
      {
        vtkPolyData* output = out.Output;
        vtkCellData* outCD = output->GetCellData();
        std::array<vtkCellData*, 3> newCD = { out.NewVertsData, out.NewLinesData,
          out.NewPolysData };

        // Reconstruct cell data
        vtkIdType offset = 0;
        for (auto& newCellTypeCD : newCD)
        {
          for (int j = 0; j < newCellTypeCD->GetNumberOfArrays(); ++j)
          {
            outCD->CopyTuples(newCellTypeCD->GetAbstractArray(j), outCD->GetAbstractArray(j),
              offset, newCellTypeCD->GetNumberOfTuples(), 0);
          }
          offset += newCellTypeCD->GetNumberOfTuples();
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
template <class TGrid>
struct UnstructuredDataWorker
{
  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, TGrid* inputGrid, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate)
  {
    UnstructuredDataFunctor<TGrid, TPointsArray> functor(
      inputGrid, pointsArray, outputPrecision, outputMP, plane, tree, origin, normal, interpolate);
    functor.BuildAccelerationStructure();
    vtkSMPTools::For(0, inputGrid->GetNumberOfCells(), functor);
  }
};

//------------------------------------------------------------------------------
int edges[12][2] = { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 }, { 7, 6 }, { 4, 7 },
  { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } };

// Process rectilinear grids with the same algo as structured grid
template <class TGrid, class TPointsArray>
struct StructuredDataFunctor : public CuttingFunctor<TPointsArray>
{
  StructuredDataFunctor(TGrid* inputGrid, TPointsArray* pointsArray, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate, bool generatePolygons)
    : CuttingFunctor<TPointsArray>(inputGrid, pointsArray, outputPrecision, outputMP, plane, tree,
        origin, normal, interpolate, generatePolygons)
  {
  }

  void Initialize() override { CuttingFunctor<TPointsArray>::Initialize(); }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    // Actual computation.
    // Note the usage of thread local objects. These objects
    // persist for each thread across multiple execution of the
    // functor.
    vtkLocalDataType& localData = this->LocalData.Local();
    vtkPointLocator* loc = localData.Locator;
    vtkPoints* newPoints = loc->GetPoints();

    vtkPointData* inPD = this->Input->GetPointData();
    vtkCellData* inCD = this->Input->GetCellData();

    vtkPolyData* output = localData.Output;
    vtkPointData* outPD = nullptr;
    vtkCellData* outCD = nullptr;

    if (this->Interpolate)
    {
      outPD = output->GetPointData();
      outCD = output->GetCellData();
    }

    vtkCellArray* newPolys = this->NewPolys.Local();

    // Loop over the cell spheres, processing those cells whose
    // bounding sphere intersect with the plane.
    vtkIdType cellI, cellJ, cellK;
    vtkIdType ptId;
    int dims[3], cellDims[3];
    TGrid* sgrid = TGrid::SafeDownCast(this->Input);
    sgrid->GetDimensions(dims);
    cellDims[0] = dims[0] - 1;
    cellDims[1] = dims[1] - 1;
    cellDims[2] = dims[2] - 1;
    vtkIdType sliceOffset = static_cast<vtkIdType>(dims[0]) * dims[1];
    vtkIdType cellSliceOffset = static_cast<vtkIdType>(cellDims[0]) * cellDims[1];
    double* planeOrigin = this->Origin;
    double* planeNormal = this->Normal;
    auto points = vtk::DataArrayTupleRange<3>(this->InPointsArray);
    const unsigned char* selected = this->Selected + beginCellId;

    static const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    EDGE_LIST* edge;
    int i, j, idx, *vert;

    // Here we have to retrieve the cell points and cell ids and do the hard work
    int v1, v2, newCellId;
    vtkIdType npts, newIds[12];
    double t, x[3], deltaScalar;
    vtkIdType p1, p2;
    vtkIdType cellIds[8];
    double s[8];

    // Traverse this batch of cells (whose bounding sphere possibly
    // intersects the plane).
    bool needCell;
    vtkIdList*& cellPointIds = this->CellPointIds.Local();
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      needCell = false;
      if (this->SphereTree)
      {
        if (*selected++)
        {
          needCell = true;
        }
      }
      else
      {
        needCell = this->IsCellSlicedByPlane(cellId, cellPointIds);
      }
      if (needCell)
      {
        cellI = cellId % cellDims[0];
        cellJ = (cellId / cellDims[0]) % cellDims[1];
        cellK = cellId / cellSliceOffset;
        ptId = cellI + cellJ * dims[0] + cellK * sliceOffset;

        cellIds[0] = ptId;
        cellIds[1] = cellIds[0] + 1;
        cellIds[2] = cellIds[0] + 1 + dims[0];
        cellIds[3] = cellIds[0] + dims[0];
        cellIds[4] = cellIds[0] + sliceOffset;
        cellIds[5] = cellIds[1] + sliceOffset;
        cellIds[6] = cellIds[2] + sliceOffset;
        cellIds[7] = cellIds[3] + sliceOffset;

        // Get the points
        for (i = 0; i < 8; ++i)
        {
          const auto& cellPoint = points[cellIds[i]];
          s[i] = (cellPoint[0] - planeOrigin[0]) * planeNormal[0] +
            (cellPoint[1] - planeOrigin[1]) * planeNormal[1] +
            (cellPoint[2] - planeOrigin[2]) * planeNormal[2];
        }

        // Return if we are not producing anything
        if ((s[0] >= 0.0 && s[1] >= 0.0 && s[2] >= 0.0 && s[3] >= 0.0 && s[4] >= 0.0 &&
              s[5] >= 0.0 && s[6] >= 0.0 && s[7] >= 0.0) ||
          (s[0] < 0.0 && s[1] < 0.0 && s[2] < 0.0 && s[3] < 0.0 && s[4] < 0.0 && s[5] < 0.0 &&
            s[6] < 0.0 && s[7] < 0.0))
        {
          continue;
        }

        // Build the case table and start producing sn output polygon as necessary
        for (i = 0, idx = 0; i < 8; ++i)
        {
          if (s[i] >= 0.0)
          {
            idx |= CASE_MASK[i];
          }
        }

        if (this->GeneratePolygons)
        {
          vtkMarchingCubesPolygonCases* polyCase = vtkMarchingCubesPolygonCases::GetCases() + idx;
          edge = polyCase->edges;
        }
        else
        {
          vtkMarchingCubesTriangleCases* triCase = vtkMarchingCubesTriangleCases::GetCases() + idx;
          edge = triCase->edges;
        }

        // Produce the intersections
        while (*edge > -1) // for all polygons
        {
          npts = this->GeneratePolygons ? *edge++ : 3;
          // start polygon/triangle edge intersections
          for (i = 0; i < npts; i++, ++edge)
          {
            vert = edges[*edge];
            deltaScalar = s[vert[1]] - s[vert[0]];
            v1 = vert[0];
            v2 = vert[1];

            // linear interpolation
            t = (deltaScalar == 0.0 ? 0.0 : (-s[v1]) / deltaScalar);

            const auto& x1 = points[cellIds[v1]];
            const auto& x2 = points[cellIds[v2]];

            for (j = 0; j < 3; j++)
            {
              x[j] = x1[j] + t * (x2[j] - x1[j]);
            }
            if ((newIds[i] = newPoints->InsertNextPoint(x)) >= 0)
            {
              if (outPD)
              {
                p1 = cellIds[v1];
                p2 = cellIds[v2];
                outPD->InterpolateEdge(inPD, newIds[i], p1, p2, t);
              }
            }
          } // for all edges of polygon/triangle

          // insert polygon
          newCellId = newPolys->InsertNextCell(npts, newIds);
          if (outCD)
          {
            outCD->CopyData(inCD, cellId, newCellId);
          }
        } // for each polygon/triangle
      }   // for all selected cells
    }     // for each cell
  }       // operator()

  void Reduce() override { CuttingFunctor<TPointsArray>::Reduce(); }
};

//------------------------------------------------------------------------------
template <class TGrid>
struct StructuredDataWorker
{
  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, TGrid* inputGrid, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate, bool generatePolygons)
  {
    StructuredDataFunctor<TGrid, TPointsArray> functor(inputGrid, pointsArray, outputPrecision,
      outputMP, plane, tree, origin, normal, interpolate, generatePolygons);
    functor.BuildAccelerationStructure();
    vtkSMPTools::For(0, inputGrid->GetNumberOfCells(), functor);
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
// Here is the VTK class proper.
// Construct object with a single contour value of 0.0.
vtkPlaneCutter::vtkPlaneCutter()
  : Plane(vtkPlane::New())
  , ComputeNormals(false)
  , InterpolateAttributes(true)
  , GeneratePolygons(true)
  , BuildTree(true)
  , BuildHierarchy(true)
  , MergePoints(false)
  , OutputPointsPrecision(DEFAULT_PRECISION)
  , DataChanged(true)
  , IsPolyDataConvex(false)
  , IsUnstructuredGrid3DLinear(false)
{
  this->InputInfo = vtkInputInfo(nullptr, 0);
}

//------------------------------------------------------------------------------
vtkPlaneCutter::~vtkPlaneCutter()
{
  this->SetPlane(nullptr);
  this->InputInfo = vtkInputInfo(nullptr, 0);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtkPlaneCutter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Plane != nullptr)
  {
    vtkMTimeType mTime2 = this->Plane->GetMTime();
    return (mTime2 > mTime ? mTime2 : mTime);
  }
  else
  {
    return mTime;
  }
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  int outputType = -1;
  if (vtkDataSet::SafeDownCast(inputDO))
  {
    outputType = VTK_POLY_DATA;
  }
  else if (vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    outputType = VTK_PARTITIONED_DATA_SET;
  }
  else if (vtkPartitionedDataSetCollection::SafeDownCast(inputDO) ||
    vtkUniformGridAMR::SafeDownCast(inputDO))
  {
    // for vtkUniformGridAMR, we create a vtkPartitionedDataSetCollection
    // because the output datasets per level will be vtkPolyData instead of vtkStructuredGrid.
    outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  }
  else if (vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    outputType = VTK_MULTIBLOCK_DATA_SET;
  }
  else
  {
    vtkErrorMacro("Unsupported input type: " << inputDO->GetClassName());
    return 0;
  }

  return vtkDataObjectAlgorithm::SetOutputDataObject(
           outputType, outputVector->GetInformationObject(0), /*exact*/ true)
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
vtkSphereTree* vtkPlaneCutter::GetSphereTree(vtkDataSet* ds)
{
  if (this->BuildTree)
  {
    auto pair =
      this->SphereTrees.insert(std::make_pair(ds, vtk::TakeSmartPointer(vtkSphereTree::New())));
    return pair.first->second.GetPointer();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
int vtkPlaneCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing plane cutter");
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  // reset sphere trees if the input has changed
  this->DataChanged = false;
  if (this->InputInfo.Input != inputDO || this->InputInfo.LastMTime != inputDO->GetMTime())
  {
    this->InputInfo = vtkInputInfo(inputDO, inputDO->GetMTime());
    this->SphereTrees.clear();
    this->DataChanged = true;
  }

  if (auto inputMB = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    auto outputMB = vtkMultiBlockDataSet::GetData(outputVector, 0);
    assert(outputMB != nullptr);
    return this->ExecuteMultiBlockDataSet(inputMB, outputMB);
  }
  else if (auto inputAMR = vtkUniformGridAMR::SafeDownCast(inputDO))
  {
    auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
    assert(outputPDC != nullptr);
    return this->ExecuteUniformGridAMR(inputAMR, outputPDC);
  }
  else if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
    assert(outputPDC != nullptr);
    return this->ExecutePartitionedDataCollection(inputPDC, outputPDC);
  }
  else if (auto inputPD = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    auto outputPD = vtkPartitionedDataSet::GetData(outputVector, 0);
    assert(outputPD != nullptr);
    return this->ExecutePartitionedData(inputPD, outputPD, true /*copyStructure*/);
  }
  else if (auto inputDS = vtkDataSet::SafeDownCast(inputDO))
  {
    auto outputPolyData = vtkPolyData::GetData(outputVector, 0);
    assert(outputPolyData != nullptr);
    return this->ExecuteDataSet(inputDS, this->GetSphereTree(inputDS), outputPolyData);
  }
  else
  {
    vtkErrorMacro("Unrecognized input type :" << inputDO->GetClassName());
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::ExecuteMultiBlockDataSet(
  vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output)
{
  assert(output != nullptr);
  output->CopyStructure(input);
  int ret = 0;
  using Opts = vtk::DataObjectTreeOptions;
  auto inputRange =
    vtk::Range(input, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
  for (auto dObj : inputRange)
  {
    vtkDataSet* inputDS = vtkDataSet::SafeDownCast(dObj);
    vtkNew<vtkPolyData> outputPolyData;
    ret += this->ExecuteDataSet(inputDS, this->GetSphereTree(inputDS), outputPolyData);
    dObj.SetDataObject(output, outputPolyData);
  }
  return ret == static_cast<int>(inputRange.size()) ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::ExecuteUniformGridAMR(
  vtkUniformGridAMR* input, vtkPartitionedDataSetCollection* output)
{
  assert(output != nullptr);
  vtkNew<vtkDataAssembly> hierarchyUnused;
  vtkNew<vtkPartitionedDataSetCollection> tempPDC;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(input, hierarchyUnused, tempPDC))
  {
    vtkErrorMacro("Failed to generate hierarchy for input!");
    return 0;
  }
  int ret = 0;
  for (unsigned int index = 0; index < tempPDC->GetNumberOfPartitionedDataSets(); ++index)
  {
    ret += this->ExecutePartitionedData(tempPDC->GetPartitionedDataSet(index),
      tempPDC->GetPartitionedDataSet(index), false /*copyStructure*/);
  }
  output->ShallowCopy(tempPDC);
  return ret == static_cast<int>(tempPDC->GetNumberOfPartitionedDataSets()) ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::ExecutePartitionedDataCollection(
  vtkPartitionedDataSetCollection* input, vtkPartitionedDataSetCollection* output)
{
  assert(output != nullptr);
  output->CopyStructure(input);
  int ret = 0;
  for (unsigned int index = 0; index < input->GetNumberOfPartitionedDataSets(); ++index)
  {
    ret += this->ExecutePartitionedData(input->GetPartitionedDataSet(index),
      output->GetPartitionedDataSet(index), false /*copyStructure*/);
  }
  return ret == static_cast<int>(input->GetNumberOfPartitionedDataSets()) ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::ExecutePartitionedData(
  vtkPartitionedDataSet* input, vtkPartitionedDataSet* output, bool copyStructure)
{
  assert(output != nullptr);
  if (copyStructure)
  {
    output->CopyStructure(input);
  }
  int ret = 0;
  for (unsigned int cc = 0, max = input->GetNumberOfPartitions(); cc < max; ++cc)
  {
    auto inputDS = input->GetPartition(cc);
    vtkNew<vtkPolyData> outputPolyData;
    ret += this->ExecuteDataSet(inputDS, this->GetSphereTree(inputDS), outputPolyData);
    output->SetPartition(cc, outputPolyData);
  }
  return ret == static_cast<int>(input->GetNumberOfPartitions()) ? 1 : 0;
}

//------------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
int vtkPlaneCutter::ExecuteDataSet(vtkDataSet* input, vtkSphereTree* tree, vtkPolyData* output)
{
  assert(output != nullptr);
  vtkPlane* plane = this->Plane;
  if (this->Plane == nullptr)
  {
    vtkDebugMacro(<< "Cutting requires vtkPlane");
    return 0;
  }

  // Check input
  vtkIdType numPts, numCells;
  if (input == nullptr || (numCells = input->GetNumberOfCells()) < 1 ||
    (numPts = input->GetNumberOfPoints()) < 1)
  {
    vtkDebugMacro("No input");
    return 1;
  }

  // Set up the cut operation
  double planeOrigin[3], planeNormal[3];
  plane->GetNormal(planeNormal);
  vtkMath::Normalize(planeNormal);
  plane->GetOrigin(planeOrigin);
  if (plane->GetTransform())
  {
    plane->GetTransform()->TransformNormalAtPoint(planeOrigin, planeNormal, planeNormal);
    plane->GetTransform()->TransformPoint(planeOrigin, planeOrigin);
  }

  // Delegate the processing to the matching algorithm. If the input data is vtkImageData,
  // then delegation to vtkFlyingEdgesPlaneCutter. If the input data is vtkPolyData, and
  // the input cells are convex polygons, then delegate to vtkPolyDataPlaneCutter. If the
  // input is an vtkUnstructuredGrid and the input cells are 3d linear, then delegate to
  // vtk3DLinearGridPlaneCutter.
  if (vtkImageData::SafeDownCast(input))
  {
    vtkDataSet* tmpInput = input;
    bool elevationFlag = false;

    // Check to see if there is a scalar associated with the image
    if (!input->GetPointData()->GetScalars())
    {
      // Add an elevation scalar
      vtkNew<vtkElevationFilter> elevation;
      elevation->SetInputData(tmpInput);
      elevation->Update();
      tmpInput = elevation->GetOutput();
      tmpInput->Register(this);
      elevationFlag = true;
    }

    // let flying edges do the work
    vtkNew<vtkFlyingEdgesPlaneCutter> planeCutter;
    vtkNew<vtkPlane> xPlane;
    xPlane->SetOrigin(planeOrigin);
    xPlane->SetNormal(planeNormal);
    planeCutter->SetPlane(xPlane);
    planeCutter->SetComputeNormals(this->ComputeNormals);
    planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
    planeCutter->SetInputData(tmpInput);
    planeCutter->Update();
    vtkDataSet* slice = planeCutter->GetOutput();
    output->ShallowCopy(slice);

    // Remove elevation data
    if (elevationFlag)
    {
      slice->GetPointData()->RemoveArray("Elevation");
      tmpInput->Delete();
    }
    else if (!this->InterpolateAttributes)
    {
      // Remove unwanted point data
      // In this case, Flying edges outputs only a single array in point data
      // scalars cannot be null
      vtkDataArray* scalars = slice->GetPointData()->GetScalars();
      slice->GetPointData()->RemoveArray(scalars->GetName());
    }
    return 1;
  }
  else if (vtkPolyData::SafeDownCast(input))
  {
    // Check whether we have convex, vtkPolyData cells. Cache the computation
    // of convexity, so it only needs be done once if the input does not change.
    if (this->DataChanged) // cache convexity check - it can be expensive
    {
      this->IsPolyDataConvex = vtkPolyDataPlaneCutter::CanFullyProcessDataObject(input);
    }
    if (this->IsPolyDataConvex)
    {
      vtkNew<vtkPlane> xPlane; // create temp transformed plane
      xPlane->SetNormal(planeNormal);
      xPlane->SetOrigin(planeOrigin);
      vtkNew<vtkPolyDataPlaneCutter> planeCutter;
      planeCutter->SetOutputPointsPrecision(this->OutputPointsPrecision);
      planeCutter->SetInputData(input);
      planeCutter->SetPlane(xPlane);
      planeCutter->SetComputeNormals(this->ComputeNormals);
      planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
      planeCutter->Update();
      vtkDataSet* outPlane = planeCutter->GetOutput();
      output->ShallowCopy(outPlane);
      return 1;
    }
  }
  else if (vtkUnstructuredGrid::SafeDownCast(input))
  {
    // Check whether we have 3d linear cells. Cache the computation
    // of linearity, so it only needs be done once if the input does not change.
    if (this->DataChanged)
    {
      this->IsUnstructuredGrid3DLinear =
        vtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(input);
    }
    if (this->IsUnstructuredGrid3DLinear)
    {
      vtkNew<vtkPlane> xPlane; // create temp transformed plane
      xPlane->SetNormal(planeNormal);
      xPlane->SetOrigin(planeOrigin);
      vtkNew<vtk3DLinearGridPlaneCutter> planeCutter;
      planeCutter->SetOutputPointsPrecision(this->OutputPointsPrecision);
      planeCutter->SetMergePoints(this->MergePoints);
      planeCutter->SetInputData(input);
      planeCutter->SetPlane(xPlane);
      planeCutter->SetComputeNormals(this->ComputeNormals);
      planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
      planeCutter->Update();
      vtkDataSet* outPlane = vtkDataSet::SafeDownCast(planeCutter->GetOutput());
      output->ShallowCopy(outPlane);
      return 1;
    }
  }

  // If here, then we use more general methods to produce the cut.
  // This means building a sphere tree.
  if (tree)
  {
    tree->SetBuildHierarchy(this->BuildHierarchy);
    tree->Build(input);
  }

  auto tempOutputMP = vtkSmartPointer<vtkMultiPieceDataSet>::New();
  // Threaded execute
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  if (auto inputSG = vtkStructuredGrid::SafeDownCast(input))
  {
    StructuredDataWorker<vtkStructuredGrid> worker;
    auto pointsArray = inputSG->GetPoints()->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputSG, this->OutputPointsPrecision,
          tempOutputMP, plane, tree, planeOrigin, planeNormal, this->InterpolateAttributes,
          this->GeneratePolygons))
    {
      worker(pointsArray, inputSG, this->OutputPointsPrecision, tempOutputMP, plane, tree,
        planeOrigin, planeNormal, this->InterpolateAttributes, this->GeneratePolygons);
    }
  }
  else if (auto inputRG = vtkRectilinearGrid::SafeDownCast(input))
  {
    StructuredDataWorker<vtkRectilinearGrid> worker;
    vtkNew<vtkPoints> points;
    inputRG->GetPoints(points);
    auto pointsArray = points->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputRG, this->OutputPointsPrecision,
          tempOutputMP, plane, tree, planeOrigin, planeNormal, this->InterpolateAttributes,
          this->GeneratePolygons))
    {
      worker(pointsArray, inputRG, this->OutputPointsPrecision, tempOutputMP, plane, tree,
        planeOrigin, planeNormal, this->InterpolateAttributes, this->GeneratePolygons);
    }
  }
  else if (auto inputPolyData = vtkPolyData::SafeDownCast(input))
  {
    UnstructuredDataWorker<vtkPolyData> worker;
    auto pointsArray = inputPolyData->GetPoints()->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputPolyData, this->OutputPointsPrecision,
          tempOutputMP, plane, tree, planeOrigin, planeNormal, this->InterpolateAttributes))
    {
      worker(pointsArray, inputPolyData, this->OutputPointsPrecision, tempOutputMP, plane, tree,
        planeOrigin, planeNormal, this->InterpolateAttributes);
    }
  }
  // get any implementations of vtkUnstructuredGridBase
  else if (auto inputUG = vtkUnstructuredGridBase::SafeDownCast(input))
  {
    UnstructuredDataWorker<vtkUnstructuredGridBase> worker;
    auto pointsArray = inputUG->GetPoints()->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputUG, this->OutputPointsPrecision,
          tempOutputMP, plane, tree, planeOrigin, planeNormal, this->InterpolateAttributes))
    {
      worker(pointsArray, inputUG, this->OutputPointsPrecision, tempOutputMP, plane, tree,
        planeOrigin, planeNormal, this->InterpolateAttributes);
    }
  }
  else
  {
    vtkErrorMacro("Unsupported Dataset type");
    return 0;
  }

  // Generate normals across all points if requested
  using Opts = vtk::DataObjectTreeOptions;
  auto tempOutputMPRange =
    vtk::Range(tempOutputMP, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
  if (this->ComputeNormals)
  {
    for (vtkDataObject* dObj : tempOutputMPRange)
    {
      vtkPlaneCutter::AddNormalArray(planeNormal, vtkPolyData::SafeDownCast(dObj));
    }
  }
  // append all pieces into one
  vtkNew<vtkAppendDataSets> append;
  append->SetOutputDataSetType(VTK_POLY_DATA);
  append->SetOutputPointsPrecision(this->OutputPointsPrecision);
  append->SetMergePoints(this->MergePoints);
  for (vtkDataObject* dObj : tempOutputMPRange)
  {
    append->AddInputData(vtkPolyData::SafeDownCast(dObj));
  }
  append->Update();
  output->ShallowCopy(append->GetOutput());
  return 1;
}

//------------------------------------------------------------------------------
void vtkPlaneCutter::AddNormalArray(double* planeNormal, vtkPolyData* polyData)
{
  vtkNew<vtkFloatArray> newNormals;
  newNormals->SetNumberOfComponents(3);
  newNormals->SetName("Normals");
  newNormals->SetNumberOfTuples(polyData->GetNumberOfPoints());
  vtkSMPTools::For(0, polyData->GetNumberOfPoints(), [&](vtkIdType begin, vtkIdType end) {
    for (vtkIdType i = begin; i < end; ++i)
    {
      newNormals->SetTuple(i, planeNormal);
    }
  });
  polyData->GetPointData()->AddArray(newNormals);
}

//------------------------------------------------------------------------------
void vtkPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
  os << indent << "Generate Polygons: " << (this->GeneratePolygons ? "On\n" : "Off\n");
  os << indent << "Build Tree: " << (this->BuildTree ? "On\n" : "Off\n");
  os << indent << "Build Hierarchy: " << (this->BuildHierarchy ? "On\n" : "Off\n");
  os << indent << "Merge Points: " << (this->MergePoints ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
