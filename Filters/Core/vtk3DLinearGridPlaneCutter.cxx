// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_7_0
#define VTK_DEPRECATION_LEVEL 0

#include "vtk3DLinearGridPlaneCutter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContour3DLinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtk3DLinearGridPlaneCutter);
vtkCxxSetObjectMacro(vtk3DLinearGridPlaneCutter, Plane, vtkPlane);

//------------------------------------------------------------------------------
// Classes to support threaded execution. Note that there is only one
// strategy at this time: a path that pre-computes plane function values and
// uses these to cull non-intersected cells. Sphere trees may be supported in
// the future.

// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq, _num, _op)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op(0, _num);                                                                                \
    }                                                                                              \
  } while (false)

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op.Initialize();                                                                            \
      _op(0, _num);                                                                                \
      _op.Reduce();                                                                                \
    }                                                                                              \
    _nt = _op.NumThreadsUsed;                                                                      \
  } while (false)

namespace
{
// Functor for assigning normals at each point
struct ComputePointNormals
{
  float Normal[3];
  float* PointNormals;
  vtk3DLinearGridPlaneCutter* Filter;

  ComputePointNormals(float normal[3], float* ptNormals, vtk3DLinearGridPlaneCutter* filter)
    : PointNormals(ptNormals)
    , Filter(filter)
  {
    this->Normal[0] = normal[0];
    this->Normal[1] = normal[1];
    this->Normal[2] = normal[2];
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    float* n = this->PointNormals + 3 * ptId;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId, n += 3)
    {
      if (ptId % checkAbortInterval == 0)
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
      n[0] = this->Normal[0];
      n[1] = this->Normal[1];
      n[2] = this->Normal[2];
    }
  }

  static void Execute(vtkPolyData* output, vtkPlane* plane, vtk3DLinearGridPlaneCutter* filter)
  {
    vtkIdType numPts = output->GetNumberOfPoints();

    vtkNew<vtkFloatArray> ptNormals;
    ptNormals->SetName("Normals");
    ptNormals->SetNumberOfComponents(3);
    ptNormals->SetNumberOfTuples(numPts);
    float* ptN = ptNormals->GetPointer(0);

    // Get the normal
    double dn[3];
    plane->GetNormal(dn);
    vtkMath::Normalize(dn);
    float n[3];
    n[0] = static_cast<float>(dn[0]);
    n[1] = static_cast<float>(dn[1]);
    n[2] = static_cast<float>(dn[2]);

    // Process all points, averaging normals
    ComputePointNormals compute(n, ptN, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), numPts, compute);

    // Clean up and get out
    output->GetPointData()->SetNormals(ptNormals);
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
// Construct an instance of the class.
vtk3DLinearGridPlaneCutter::vtk3DLinearGridPlaneCutter()
{
  this->Plane = vtkPlane::New();
  this->MergePoints = false;
  this->InterpolateAttributes = true;
  this->GenerateCutScalars = 0;
  this->ComputeNormals = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->SequentialProcessing = false;
  this->GenerateTriangles = true;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;
}

//------------------------------------------------------------------------------
vtk3DLinearGridPlaneCutter::~vtk3DLinearGridPlaneCutter()
{
  this->SetPlane(nullptr);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtk3DLinearGridPlaneCutter::GetMTime()
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
void vtk3DLinearGridPlaneCutter::SetMergePointsOn()
{
  this->SetMergePoints(true);
}

//------------------------------------------------------------------------------
void vtk3DLinearGridPlaneCutter::SetMergePointsOff()
{
  this->SetMergePoints(false);
}

//------------------------------------------------------------------------------
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridPlaneCutter::ProcessPiece(
  vtkUnstructuredGrid* input, vtkPlane* plane, vtkPolyData* output)
{
  // Make sure there is input data to process
  if (!input || !plane || !output)
  {
    vtkLog(TRACE, "Null input, plane, or output");
    return 1;
  }

  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = inPts ? inPts->GetNumberOfPoints() : 0;
  vtkCellArray* cells = input->GetCells();
  vtkIdType numCells = cells ? cells->GetNumberOfCells() : 0;
  if (numPts <= 0 || numCells <= 0)
  {
    vtkLog(TRACE, "Empty input");
    return 1;
  }

  // Compute plane-cut scalars
  vtkNew<vtkAOSDataArrayTemplate<double>> distanceArray;
  distanceArray->SetName("cutScalars");
  distanceArray->SetNumberOfValues(numPts);
  plane->FunctionValue(input->GetPoints()->GetData(), distanceArray);

  // copy to add the plane distance array
  vtkNew<vtkUnstructuredGrid> inputCopy;
  inputCopy->ShallowCopy(input);
  inputCopy->GetPointData()->AddArray(distanceArray);

  // execute contour
  vtkNew<vtkContour3DLinearGrid> contour;
  contour->SetInputData(inputCopy);
  contour->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, distanceArray->GetName());
  contour->SetValue(0, 0.0);
  contour->SetInterpolateAttributes(this->InterpolateAttributes);
  contour->SetComputeNormals(false);
  contour->SetComputeScalars(this->GenerateCutScalars);
  contour->SetOutputPointsPrecision(this->OutputPointsPrecision);
  contour->SetSequentialProcessing(this->SequentialProcessing);
  contour->SetGenerateTriangles(this->GenerateTriangles);
  contour->SetContainerAlgorithm(this);
  contour->Update();

  output->ShallowCopy(contour->GetOutput());
  if (this->ComputeNormals)
  {
    ComputePointNormals::Execute(output, plane, this);
  }

  return 1;
}

//------------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int vtk3DLinearGridPlaneCutter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    if (vtkPolyData::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a vtkMultiBlockDataSet as output;
    if (vtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  vtkLog(ERROR, "Not sure what type of output to create!");
  return 0;
}

//------------------------------------------------------------------------------
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridPlaneCutter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* inputGrid =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* outputPolyData =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCompositeDataSet* inputCDS =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkMultiBlockDataSet* outputMBDS =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputPolyData == nullptr) &&
    (inputCDS == nullptr || outputMBDS == nullptr))
  {
    return 0;
  }

  // Need a plane to do the cutting
  vtkPlane* plane = this->Plane;
  if (!plane)
  {
    vtkLog(ERROR, "Cut plane not defined");
    return 0;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output vtkPolyData.
  if (inputGrid)
  {
    this->ProcessPiece(inputGrid, plane, outputPolyData);
    this->CheckAbort();
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a vtkPolyData that is added to
  // the output multiblock dataset.
  else
  {
    vtkUnstructuredGrid* grid;
    vtkPolyData* polydata;
    outputMBDS->CopyStructure(inputCDS);
    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (this->GetAbortOutput())
      {
        break;
      }
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = vtkUnstructuredGrid::SafeDownCast(ds)))
      {
        polydata = vtkPolyData::New();
        this->ProcessPiece(grid, plane, polydata);
        outputMBDS->SetDataSet(inIter, polydata);
        polydata->Delete();
      }
      else
      {
        vtkLog(TRACE, "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(vtkDataObject* object)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    // Get list of cell types in the unstructured grid
    if (vtkUnsignedCharArray* cellTypes = ug->GetDistinctCellTypesArray())
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        if (cellType != VTK_EMPTY_CELL && cellType != VTK_VOXEL && cellType != VTK_TETRA &&
          cellType != VTK_HEXAHEDRON && cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
        {
          // Unsupported cell type, can't process data
          return false;
        }
      }
    }

    // All cell types are supported, can process data.
    return true;
  }
  else if (cd)
  {
    bool supported = true;
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto leafDS = iter->GetCurrentDataObject();
      if (!CanFullyProcessDataObject(leafDS))
      {
        supported = false;
        break;
      }
    }
    return supported;
  }

  return false; // not a vtkUnstructuredGrid nor a composite dataset
}

//------------------------------------------------------------------------------
int vtk3DLinearGridPlaneCutter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtk3DLinearGridPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";

  os << indent << "Merge Points: " << (this->MergePoints ? "true\n" : "false\n");
  os << indent << "Generate Cut Scalars: " << (this->GenerateCutScalars ? "true\n" : "false\n");
  os << indent
     << "Interpolate Attributes: " << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "true\n" : "false\n");
  os << indent << "Generate Triangles: " << (this->GenerateTriangles ? "true\n" : "false\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: " << (this->LargeIds ? "true\n" : "false\n");
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
VTK_ABI_NAMESPACE_END
