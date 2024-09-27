// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlaneCutter.h"

#include "vtk3DLinearGridPlaneCutter.h"
#include "vtkAppendDataSets.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvertToMultiBlockDataSet.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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
#include "vtkStructuredDataPlaneCutter.h"
#include "vtkStructuredGrid.h"
#include "vtkTransform.h"
#include "vtkUniformGridAMR.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
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
  vtkPlaneCutter* Filter;

  CuttingFunctor(vtkDataSet* input, TPointsArray* pointsArray, int outputPrecision,
    vtkMultiPieceDataSet* outputMP, vtkPlane* plane, vtkSphereTree* tree, double* origin,
    double* normal, bool interpolate, vtkPlaneCutter* filter)
    : Input(input)
    , InPointsArray(pointsArray)
    , OutputMP(outputMP)
    , Plane(plane)
    , SphereTree(tree)
    , InOutArray(nullptr)
    , OutputPrecision(outputPrecision)
    , Origin(origin)
    , Normal(normal)
    , Interpolate(interpolate)
    , Filter(filter)
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
    double* normal, bool interpolate, vtkPlaneCutter* filter)
    : CuttingFunctor<TPointsArray>(inputGrid, pointsArray, outputPrecision, outputMP, plane, tree,
        origin, normal, interpolate, filter)
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
    bool isFirst = vtkSMPTools::GetSingleThread();

    vtkIdList*& cellPointIds = this->CellPointIds.Local();
    vtkIdType checkAbortInterval = std::min((endCellId - beginCellId) / 10 + 1, (vtkIdType)1000);
    // Loop over the cell, processing only the one that are needed
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }

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
    double* normal, bool interpolate, vtkPlaneCutter* filter)
  {
    UnstructuredDataFunctor<TGrid, TPointsArray> functor(inputGrid, pointsArray, outputPrecision,
      outputMP, plane, tree, origin, normal, interpolate, filter);
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
  else if (vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  }
  else if (vtkMultiBlockDataSet::SafeDownCast(inputDO) || vtkUniformGridAMR::SafeDownCast(inputDO))
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
// This method delegates to the appropriate algorithm
int vtkPlaneCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing plane cutter");
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  if (inputDO == nullptr)
  {
    vtkErrorMacro("Input is nullptr");
    return 0;
  }
  // reset cached info if the input has changed
  this->DataChanged = false;
  if (this->InputInfo.Input != inputDO || this->InputInfo.LastMTime != inputDO->GetMTime())
  {
    this->InputInfo = vtkInputInfo(inputDO, inputDO->GetMTime());
    this->SphereTrees.clear();
    this->CanBeFullyProcessed.clear();
    this->DataChanged = true;
  }

  if (auto inputDOT = vtkDataObjectTree::SafeDownCast(inputDO))
  {
    auto outputDOT = vtkDataObjectTree::SafeDownCast(outputDO);
    assert(outputDOT != nullptr);
    return this->ExecuteDataObjectTree(inputDOT, outputDOT);
  }
  else if (vtkUniformGridAMR::SafeDownCast(inputDO))
  {
    vtkNew<vtkConvertToMultiBlockDataSet> toMBDS;
    toMBDS->SetInputData(inputDO);
    toMBDS->Update();
    auto convertInputDOT = vtkMultiBlockDataSet::SafeDownCast(toMBDS->GetOutput());
    auto outputDOT = vtkDataObjectTree::SafeDownCast(outputDO);
    assert(outputDOT != nullptr);
    return this->ExecuteDataObjectTree(convertInputDOT, outputDOT);
  }
  else if (auto inputDS = vtkDataSet::SafeDownCast(inputDO))
  {
    auto outputPolyData = vtkPolyData::GetData(outputVector, 0);
    assert(outputPolyData != nullptr);
    return this->ExecuteDataSet(inputDS, outputPolyData);
  }
  else
  {
    vtkErrorMacro("Unrecognized input type :" << inputDO->GetClassName());
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkPlaneCutter::ExecuteDataObjectTree(vtkDataObjectTree* input, vtkDataObjectTree* output)
{
  output->CopyStructure(input);
  int ret = 0;
  using Opts = vtk::DataObjectTreeOptions;
  auto inputRange =
    vtk::Range(input, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
  for (auto dObj : inputRange)
  {
    vtkDataSet* inputDS = vtkDataSet::SafeDownCast(dObj);
    vtkNew<vtkPolyData> outputPolyData;
    ret += this->ExecuteDataSet(inputDS, outputPolyData);
    dObj.SetDataObject(output, outputPolyData);
  }
  return ret == static_cast<int>(inputRange.size()) ? 1 : 0;
}

//------------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
int vtkPlaneCutter::ExecuteDataSet(vtkDataSet* input, vtkPolyData* output)
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

  // Get Cached info (sphere tree and can be fully processed)
  vtkSphereTree* sphereTree = nullptr;
  if (this->BuildTree)
  {
    auto pair =
      this->SphereTrees.insert(std::make_pair(input, vtk::TakeSmartPointer(vtkSphereTree::New())));
    sphereTree = pair.first->second.GetPointer();
  }
  bool& canBeFullyProcessed =
    this->CanBeFullyProcessed.insert(std::make_pair(input, false)).first->second;

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

  // Delegate the processing to the matching algorithm. If the input data is vtkImageData/
  // vtkRectilinearGrid/vtkStructuredGrid, then delegate to vtkStructuredDataPlaneCutter.
  // If the input data is vtkPolyData, and the input cells are convex polygons, then delegate
  // to vtkPolyDataPlaneCutter. If the input is an vtkUnstructuredGrid and the input cells are
  // 3d linear, then delegate to vtk3DLinearGridPlaneCutter.
  if (vtkImageData::SafeDownCast(input) || vtkStructuredGrid::SafeDownCast(input) ||
    vtkRectilinearGrid::SafeDownCast(input))
  {
    vtkNew<vtkPlane> xPlane; // create temp transformed plane
    xPlane->SetNormal(planeNormal);
    xPlane->SetOrigin(planeOrigin);
    vtkNew<vtkStructuredDataPlaneCutter> planeCutter;
    planeCutter->SetOutputPointsPrecision(this->OutputPointsPrecision);
    planeCutter->SetInputData(input);
    planeCutter->SetPlane(xPlane);
    planeCutter->SetSphereTree(sphereTree);
    planeCutter->SetBuildTree(this->BuildTree);
    planeCutter->SetBuildHierarchy(this->BuildHierarchy);
    planeCutter->SetGeneratePolygons(this->GeneratePolygons);
    planeCutter->SetComputeNormals(this->ComputeNormals);
    planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
    planeCutter->SetContainerAlgorithm(this);
    planeCutter->Update();
    vtkDataSet* outPlane = planeCutter->GetOutput();
    output->ShallowCopy(outPlane);
    return 1;
  }
  else if (vtkPolyData::SafeDownCast(input))
  {
    // Check whether we have convex, vtkPolyData cells. Cache the computation
    // of convexity, so it only needs be done once if the input does not change.
    if (this->DataChanged) // cache convexity check - it can be expensive
    {
      canBeFullyProcessed = vtkPolyDataPlaneCutter::CanFullyProcessDataObject(input);
    }
    if (canBeFullyProcessed)
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
      planeCutter->SetContainerAlgorithm(this);
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
      canBeFullyProcessed = vtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(input);
    }
    if (canBeFullyProcessed)
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
      planeCutter->SetContainerAlgorithm(this);
      planeCutter->Update();
      vtkDataSet* outPlane = vtkDataSet::SafeDownCast(planeCutter->GetOutput());
      output->ShallowCopy(outPlane);
      return 1;
    }
  }

  // If here, then we use more general methods to produce the cut.
  // This means building a sphere tree.
  if (sphereTree)
  {
    sphereTree->SetBuildHierarchy(this->BuildHierarchy);
    sphereTree->Build(input);
  }

  auto tempOutputMP = vtkSmartPointer<vtkMultiPieceDataSet>::New();
  // Threaded execute
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  if (auto inputPolyData = vtkPolyData::SafeDownCast(input))
  {
    UnstructuredDataWorker<vtkPolyData> worker;
    auto pointsArray = inputPolyData->GetPoints()->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputPolyData, this->OutputPointsPrecision,
          tempOutputMP, plane, sphereTree, planeOrigin, planeNormal, this->InterpolateAttributes,
          this))
    {
      worker(pointsArray, inputPolyData, this->OutputPointsPrecision, tempOutputMP, plane,
        sphereTree, planeOrigin, planeNormal, this->InterpolateAttributes, this);
    }
  }
  // get any implementations of vtkUnstructuredGridBase
  else if (auto inputUG = vtkUnstructuredGridBase::SafeDownCast(input))
  {
    UnstructuredDataWorker<vtkUnstructuredGridBase> worker;
    auto pointsArray = inputUG->GetPoints()->GetData();
    if (!Dispatcher::Execute(pointsArray, worker, inputUG, this->OutputPointsPrecision,
          tempOutputMP, plane, sphereTree, planeOrigin, planeNormal, this->InterpolateAttributes,
          this))
    {
      worker(pointsArray, inputUG, this->OutputPointsPrecision, tempOutputMP, plane, sphereTree,
        planeOrigin, planeNormal, this->InterpolateAttributes, this);
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
  append->SetContainerAlgorithm(this);
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
  vtkSMPTools::For(0, polyData->GetNumberOfPoints(),
    [&](vtkIdType begin, vtkIdType end)
    {
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

//------------------------------------------------------------------------------
void vtkPlaneCutter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // the SphereTrees share our input and can be part of a reference loop
  for (auto pit = this->SphereTrees.begin(); pit != this->SphereTrees.end(); ++pit)
  {
    vtkGarbageCollectorReport(collector, pit->second, "SphereTree");
  }
}
VTK_ABI_NAMESPACE_END
