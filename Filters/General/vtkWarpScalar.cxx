// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWarpScalar.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMarkBoundaryFilter.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWeakPointer.h"

#include "vtkNew.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <bitset>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWarpScalar);

//------------------------------------------------------------------------------
vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkWarpScalar::~vtkWarpScalar() = default;

//------------------------------------------------------------------------------
int vtkWarpScalar::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkWarpScalar::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (this->GenerateEnclosure)
  {
    vtkStructuredGrid* inStruct = vtkStructuredGrid::GetData(inputVector[0]);
    if (inImage || inRect || inStruct)
    {
      vtkUnstructuredGrid* outUG = vtkUnstructuredGrid::GetData(outputVector);
      if (!outUG)
      {
        vtkNew<vtkUnstructuredGrid> newOutput;
        outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      }
      return 1;
    }
  }

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

// Core methods to scale points with scalars
namespace
{ // anonymous

struct ScaleWorker
{
  template <typename InPT, typename OutPT, typename ST>
  void operator()(InPT* inPts, OutPT* outPts, ST* scalars, vtkWarpScalar* self, double sf,
    bool XYPlane, vtkDataArray* inNormals, double* normal)

  {
    vtkIdType numPts = inPts->GetNumberOfTuples();
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);
    const auto sRange = vtk::DataArrayTupleRange(scalars);

    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially.
    vtkSMPTools::For(0, numPts, vtkSMPTools::THRESHOLD, [&](vtkIdType ptId, vtkIdType endPtId) {
      double s, *n = normal, inNormal[3];
      bool isFirst = vtkSMPTools::GetSingleThread();
      for (; ptId < endPtId; ++ptId)
      {
        if (isFirst)
        {
          self->CheckAbort();
        }
        if (self->GetAbortOutput())
        {
          break;
        }
        const auto xi = ipts[ptId];
        auto xo = opts[ptId];

        if (XYPlane)
        {
          s = xi[2];
        }
        else
        {
          const auto sval = sRange[ptId];
          s = sval[0]; // 0th component of the tuple
        }

        if (inNormals)
        {
          inNormals->GetTuple(ptId, inNormal);
          n = inNormal;
        }

        xo[0] = xi[0] + sf * s * n[0];
        xo[1] = xi[1] + sf * s * n[1];
        xo[2] = xi[2] + sf * s * n[2];
      }
    }); // lambda
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkWarpScalar::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->SetContainerAlgorithm(this);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->SetContainerAlgorithm(this);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  bool inputHasBoundary = false;
  vtkSmartPointer<vtkUnsignedCharArray> boundaryPoints;
  vtkSmartPointer<vtkUnsignedCharArray> boundaryCells;
  vtkSmartPointer<vtkIdTypeArray> boundaryFaceIndexes;
  if (this->GenerateEnclosure)
  {
    unsigned int dim = this->GetInputDimension(input);
    if (dim > 2)
    {
      vtkWarningMacro(
        "Cannot use GenerateEnclosure option with data set with more than 2 spatial dimensions");
    }
    else
    {
      vtkNew<vtkMarkBoundaryFilter> markBoundary;
      markBoundary->SetInputData(input);
      markBoundary->GenerateBoundaryFacesOn();
      markBoundary->SetContainerAlgorithm(this);
      markBoundary->Update();
      vtkPointSet* ptSet = vtkPointSet::SafeDownCast(markBoundary->GetOutputDataObject(0));
      if (!ptSet)
      {
        vtkErrorMacro("Output of mark boundaries is not point set");
        return 0;
      }
      boundaryPoints = vtkArrayDownCast<vtkUnsignedCharArray>(
        ptSet->GetPointData()->GetArray(markBoundary->GetBoundaryPointsName()));
      boundaryCells = vtkArrayDownCast<vtkUnsignedCharArray>(
        ptSet->GetCellData()->GetArray(markBoundary->GetBoundaryCellsName()));
      boundaryFaceIndexes = vtkArrayDownCast<vtkIdTypeArray>(
        ptSet->GetCellData()->GetArray(markBoundary->GetBoundaryFacesName()));
      if (!boundaryPoints || !boundaryCells || !boundaryFaceIndexes)
      {
        vtkErrorMacro("Could not extract boundary arrays");
        return 0;
      }
      auto range = vtk::DataArrayValueRange<1>(boundaryCells);
      inputHasBoundary = (std::find_if(range.begin(), range.end(),
                            [](unsigned char b) { return b != 0; }) != range.end());
    }
  }

  vtkPoints* inPts;
  vtkDataArray* inNormals;
  vtkDataArray* inScalars;
  vtkIdType numPts;

  vtkDebugMacro(<< "Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inPts = input->GetPoints();
  inNormals = input->GetPointData()->GetNormals();
  inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inPts || !inScalars)
  {
    vtkDebugMacro(<< "No data to warp");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  // Create the output points. Backward compatibility requires the
  // output type to be float - this can be overridden.
  vtkNew<vtkPoints> newPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION ||
    this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numPts);
  output->SetPoints(newPts);

  // Figure out what normal to use
  double normal[3] = { 0.0, 0.0, 0.0 };
  if (inNormals && !this->UseNormal)
  {
    vtkDebugMacro(<< "Using data normals");
  }
  else if (this->XYPlane)
  {
    inNormals = nullptr;
    normal[2] = 1.0;
    vtkDebugMacro(<< "Using x-y plane normal");
  }
  else
  {
    inNormals = nullptr;
    normal[0] = this->Normal[0];
    normal[1] = this->Normal[1];
    normal[2] = this->Normal[2];
    vtkDebugMacro(<< "Using Normal instance variable");
  }

  // Dispatch over point and scalar types
  using vtkArrayDispatch::Reals;
  using ScaleDispatch = vtkArrayDispatch::Dispatch3ByValueType<Reals, Reals, Reals>;
  ScaleWorker scaleWorker;
  if (!ScaleDispatch::Execute(inPts->GetData(), newPts->GetData(), inScalars, scaleWorker, this,
        this->ScaleFactor, this->XYPlane, inNormals, normal))
  { // fallback to slowpath
    scaleWorker(inPts->GetData(), newPts->GetData(), inScalars, this, this->ScaleFactor,
      this->XYPlane, inNormals, normal);
  }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  if (this->GenerateEnclosure && inputHasBoundary)
  {
    vtkPolyData* polyOutput = vtkPolyData::SafeDownCast(output);
    vtkUnstructuredGrid* ugOutput = vtkUnstructuredGrid::SafeDownCast(output);
    if (!polyOutput && !ugOutput)
    {
      vtkErrorMacro("Tried to create sidewalls on unsupported output: must be either "
                    "vtkPolyData or vtkUnstructuredGrid");
      return 0;
    }
    output->GetPoints()->InsertPoints(
      output->GetNumberOfPoints(), input->GetNumberOfPoints(), 0, input->GetPoints());
    vtkWeakPointer<vtkCellArray> topology =
      polyOutput ? polyOutput->GetPolys() : ugOutput->GetCells();
    // append the topology to itself with an offset reconstructing the original data set
    if (!topology)
    {
      vtkErrorMacro("Could not recover topology from output");
      return 0;
    }
    vtkNew<vtkCellArray> topCopy;
    topCopy->DeepCopy(topology);
    topCopy->Append(topology, input->GetNumberOfPoints());
    if (polyOutput)
    {
      polyOutput->SetPolys(topCopy);
    }
    else
    {
      vtkNew<vtkUnsignedCharArray> cTypes;
      // append types to themselves too
      cTypes->DeepCopy(ugOutput->GetCellTypesArray());
      vtkIdType typeSize = cTypes->GetNumberOfTuples();
      cTypes->InsertTuples(typeSize, typeSize, 0, cTypes);
      // update the output UG
      ugOutput->SetEditable(true);
      ugOutput->SetCells(cTypes, topCopy);
    }
    this->AppendArrays(output->GetPointData());
    this->AppendArrays(output->GetCellData());
    this->BuildSideWalls(output, input->GetNumberOfPoints(), boundaryCells, boundaryFaceIndexes);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}

//------------------------------------------------------------------------------
namespace
{
struct DimensionWorklet
{
  int maxDim = 0;
  vtkSMPThreadLocal<int> localMaxDim;
  vtkDataSet* input = nullptr;

  DimensionWorklet(vtkDataSet* inDS) { this->input = inDS; }

  void Initialize() { this->localMaxDim.Local() = 0; }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (!this->input)
    {
      return;
    }
    vtkNew<vtkGenericCell> cell;
    for (vtkIdType iCell = begin; iCell < end; iCell++)
    {
      this->input->GetCell(iCell, cell);
      int locDim = cell->GetCellDimension();
      this->localMaxDim.Local() = std::max(locDim, this->localMaxDim.Local());
      if (this->localMaxDim.Local() == 3)
      {
        break;
      }
    }
  }

  void Reduce()
  {
    std::for_each(this->localMaxDim.begin(), this->localMaxDim.end(),
      [&](int locMax) { this->maxDim = std::max(this->maxDim, locMax); });
  }
};
}

//------------------------------------------------------------------------------
unsigned int vtkWarpScalar::GetInputDimension(vtkDataSet* input)
{
  // Ensure that the call to BuildCells is made before the SMP dispatch through a dummy GetCell call
  {
    vtkNew<vtkGenericCell> genCell;
    input->GetCell(0, genCell);
  }
  ::DimensionWorklet worker(input);
  vtkSMPTools::For(0, input->GetNumberOfCells(), worker);
  return static_cast<unsigned int>(worker.maxDim);
}

//------------------------------------------------------------------------------
void vtkWarpScalar::BuildSideWalls(vtkPointSet* output, int nInputPoints,
  vtkUnsignedCharArray* boundaryCells, vtkIdTypeArray* boundaryFaceIndexes)
{
  vtkPolyData* polyOutput = vtkPolyData::SafeDownCast(output);
  vtkUnstructuredGrid* ugOutput = vtkUnstructuredGrid::SafeDownCast(output);
  assert(polyOutput || ugOutput); // already tested in calling method

  vtkNew<vtkIdList> newCell;
  newCell->SetNumberOfIds(4);
  vtkIdType iCell = 0;
  auto bFlagRange = vtk::DataArrayValueRange<1>(boundaryCells);
  auto bFaceRange = vtk::DataArrayValueRange<1>(boundaryFaceIndexes);
  auto faceIter = bFaceRange.begin();
  for (auto val : bFlagRange)
  {
    if (val)
    {
      constexpr std::size_t nBitsvtkIdType = sizeof(vtkIdType) * CHAR_BIT;
      std::bitset<nBitsvtkIdType> faceMask = *faceIter;
      vtkCell* bCell = output->GetCell(iCell);
      int nEdges = std::min(bCell->GetNumberOfEdges(), static_cast<int>(nBitsvtkIdType));
      for (int iEdge = 0; iEdge < nEdges; iEdge++)
      {
        if (faceMask[iEdge])
        {
          vtkCell* bEdge = bCell->GetEdge(iEdge);
          vtkIdList* pts = bEdge->GetPointIds();
          for (int iP = 0; iP < 2; iP++)
          {
            newCell->SetId(iP, pts->GetId(iP));
            newCell->SetId(iP + 2, pts->GetId(1 - iP) + nInputPoints);
          }

          if (polyOutput)
          {
            polyOutput->InsertNextCell(VTK_QUAD, newCell);
          }
          else
          {
            ugOutput->InsertNextCell(VTK_QUAD, newCell);
          }

          for (int iArr = 0; iArr < output->GetCellData()->GetNumberOfArrays(); iArr++)
          {
            vtkAbstractArray* aa = output->GetCellData()->GetAbstractArray(iArr);
            aa->InsertNextTuple(iCell, aa);
          }
        }
      }
    }
    iCell++;
    faceIter++;
  }
}

//------------------------------------------------------------------------------
void vtkWarpScalar::AppendArrays(vtkDataSetAttributes* setData)
{
  std::vector<vtkSmartPointer<vtkAbstractArray>> buffer(setData->GetNumberOfArrays());
  for (int iArr = 0; iArr < setData->GetNumberOfArrays(); iArr++)
  {
    vtkAbstractArray* aa = setData->GetArray(iArr);
    vtkSmartPointer<vtkAbstractArray> newAa = vtk::TakeSmartPointer(aa->NewInstance());
    newAa->DeepCopy(aa);
    newAa->InsertTuples(newAa->GetNumberOfTuples(), aa->GetNumberOfTuples(), 0, aa);
    buffer[iArr] = newAa;
  }
  // This operation will replace all arrays in the data attributes with their doubly deep copied
  // counterparts
  std::for_each(
    buffer.begin(), buffer.end(), [&setData](vtkAbstractArray* arr) { setData->AddArray(arr); });
}

VTK_ABI_NAMESPACE_END
