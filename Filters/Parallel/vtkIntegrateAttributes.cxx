// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIntegrateAttributes.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntegrateAttributesFieldList.h"
#include "vtkIntegrationLinearStrategy.h"
#include "vtkIntegrationStrategy.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIntegrateAttributes);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkIntegrateAttributes, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkIntegrateAttributes, IntegrationStrategy, vtkIntegrationStrategy);

//------------------------------------------------------------------------------
vtkIntegrateAttributes::vtkIntegrateAttributes()
  : Controller(nullptr)
  , DivideAllCellDataByVolume(false)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  vtkNew<vtkIntegrationLinearStrategy> linearStrategy;
  this->SetIntegrationStrategy(linearStrategy);
}

//------------------------------------------------------------------------------
vtkIntegrateAttributes::~vtkIntegrateAttributes()
{
  this->SetController(nullptr);
  this->SetIntegrationStrategy(nullptr);
}

//------------------------------------------------------------------------------
vtkExecutive* vtkIntegrateAttributes::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
int vtkIntegrateAttributes::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkIntegrateAttributes::CompareIntegrationDimension(vtkDataSet* output, int dim,
  double& totalSum, double totalSumCenter[3], int& integrationDimension)
{
  // higher dimension prevails
  if (integrationDimension < dim)
  { // Throw out results from lower dimension.
    totalSum = 0;
    totalSumCenter[0] = totalSumCenter[1] = totalSumCenter[2] = 0.0;
    vtkIntegrateAttributes::ZeroAttributes(output->GetPointData());
    vtkIntegrateAttributes::ZeroAttributes(output->GetCellData());
    integrationDimension = dim;
    return 1;
  }
  // Skip this cell if we are inetrgrting a higher dimension.
  return (integrationDimension == dim);
}

//------------------------------------------------------------------------------
class vtkIntegrateAttributes::vtkIntegrateAttributesFunctor
{
private:
  // inputs
  vtkIntegrateAttributes* Self;
  vtkDataSet* Input;
  vtkUnstructuredGrid* Output;
  int TotalIntegrationDimension;
  int FieldListIndex;
  vtkIntegrateAttributesFieldList& PointFieldList;
  vtkIntegrateAttributesFieldList& CellFieldList;

  // input information
  unsigned char* Ghost;
  vtkSmartPointer<vtkIntegrationStrategy> IntegrationStrategy;

  // thread local data
  vtkSMPThreadLocalObject<vtkUnstructuredGrid> TLOutput;
  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
  vtkSMPThreadLocalObject<vtkIdList> TLCellPointIds;
  vtkSMPThreadLocal<double> TLSum;
  vtkSMPThreadLocal<std::array<double, 3>> TLSumCenter;

  // results
  double Sum;
  double SumCenter[3];

public:
  vtkIntegrateAttributesFunctor(vtkIntegrateAttributes* self, vtkDataSet* input,
    vtkUnstructuredGrid* output, int totalIntegrationDimension, int fieldListIndex,
    vtkIntegrateAttributesFieldList& pdList, vtkIntegrateAttributesFieldList& cdList,
    vtkIntegrationStrategy* strategy)
    : Self(self)
    , Input(input)
    , Output(output)
    , TotalIntegrationDimension(totalIntegrationDimension)
    , FieldListIndex(fieldListIndex)
    , PointFieldList(pdList)
    , CellFieldList(cdList)
    , Ghost(input->GetCellGhostArray() ? input->GetCellGhostArray()->GetPointer(0) : nullptr)
    , IntegrationStrategy(strategy)
  {
    if (this->Input->GetNumberOfCells() > 0)
    {
      // initialize internal data structures
      vtkNew<vtkGenericCell> cell;
      this->Input->GetCell(0, cell);
      this->IntegrationStrategy->Initialize(input);
    }
  }

  void Initialize()
  {
    this->TLSum.Local() = 0;
    auto& sumCenter = this->TLSumCenter.Local();
    sumCenter[0] = sumCenter[1] = sumCenter[2] = 0.0;
    auto output = this->TLOutput.Local();
    output->GetPointData()->DeepCopy(this->Output->GetPointData());
    vtkIntegrateAttributes::InitializeAttributes(output->GetPointData());
    output->GetCellData()->DeepCopy(this->Output->GetCellData());
    vtkIntegrateAttributes::InitializeAttributes(output->GetCellData());
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& output = this->TLOutput.Local();
    auto& cell = this->TLCell.Local();
    auto& cellPointIds = this->TLCellPointIds.Local();
    auto& sum = this->TLSum.Local();
    auto sumCenter = this->TLSumCenter.Local().data();
    vtkIdType npts;
    const vtkIdType* pts;
    int cellType, cellDim;

    const bool isFirst = vtkSMPTools::GetSingleThread();
    const auto checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Self->CheckAbort();
        }
        if (this->Self->GetAbortOutput())
        {
          break;
        }
      }
      // Make sure we are not integrating ghost/blanked cells.
      if (this->Ghost &&
        (this->Ghost[cellId] &
          (vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::HIDDENCELL)))
      {
        continue;
      }

      // get cell type
      cellType = this->Input->GetCellType(cellId);
      // skip cells that have different(lower) dimension compared to the max spatial dimension
      cellDim = vtkCellTypes::GetDimension(cellType);
      if (cellDim == 0 || this->TotalIntegrationDimension != cellDim)
      {
        continue;
      }

      switch (cellType)
      {
        // skip empty or 0D Cells
        case VTK_EMPTY_CELL:
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
          break;

        case VTK_POLY_LINE:
        case VTK_LINE:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegratePolyLine(this->Input, output, cellId, npts, pts, sum,
            sumCenter, this->CellFieldList, this->PointFieldList, this->FieldListIndex);
        }
        break;

        case VTK_TRIANGLE:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateTriangle(this->Input, output, cellId, pts[0], pts[1],
            pts[2], sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        case VTK_TRIANGLE_STRIP:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateTriangleStrip(this->Input, output, cellId, npts, pts,
            sum, sumCenter, this->CellFieldList, this->PointFieldList, this->FieldListIndex);
        }
        break;

        case VTK_POLYGON:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegratePolygon(this->Input, output, cellId, npts, pts, sum,
            sumCenter, this->CellFieldList, this->PointFieldList, this->FieldListIndex);
        }
        break;

        case VTK_PIXEL:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegratePixel(this->Input, output, cellId, npts, pts, sum,
            sumCenter, this->CellFieldList, this->PointFieldList, this->FieldListIndex);
        }
        break;

        case VTK_QUAD:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateQuad(this->Input, output, cellId, pts[0], pts[1],
            pts[2], pts[3], sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        case VTK_VOXEL:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateVoxel(this->Input, output, cellId, npts, pts, sum,
            sumCenter, this->CellFieldList, this->PointFieldList, this->FieldListIndex);
        }
        break;

        case VTK_TETRA:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateTetrahedron(this->Input, output, cellId, pts[0],
            pts[1], pts[2], pts[3], sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        case VTK_HEXAHEDRON:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateHexahedron(this->Input, output, cell, cellId, npts,
            pts, cellPointIds, sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        case VTK_WEDGE:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateWedge(this->Input, output, cell, cellId, npts, pts,
            cellPointIds, sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        case VTK_PYRAMID:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegratePyramid(this->Input, output, cell, cellId, npts, pts,
            cellPointIds, sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
        break;

        default:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrationStrategy->IntegrateDefault(this->Input, output, cell, cellId, npts,
            cellPointIds, sum, sumCenter, this->CellFieldList, this->PointFieldList,
            this->FieldListIndex);
        }
      }
    }
  }

  void Reduce()
  {
    // compute sum
    this->Sum = 0;
    for (const auto& sum : this->TLSum)
    {
      this->Sum += sum;
    }
    // compute sum center
    this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
    for (const auto& sumCenter : this->TLSumCenter)
    {
      vtkMath::Add(sumCenter, this->SumCenter, this->SumCenter);
    }
    // compute point/cell data
    for (const auto& output : this->TLOutput)
    {
      vtkIntegrateAttributes::IntegrateSatelliteData(
        output->GetPointData(), this->Output->GetPointData());
      vtkIntegrateAttributes::IntegrateSatelliteData(
        output->GetCellData(), this->Output->GetCellData());
    }
  }

  double GetSum() { return this->Sum; }

  double* GetSumCenter() { return this->SumCenter; }
};

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::ExecuteBlock(vtkDataSet* input, vtkUnstructuredGrid* output,
  int fieldset_index, vtkIntegrateAttributesFieldList& pdList,
  vtkIntegrateAttributesFieldList& cdList, double& totalSum, double totalSumCenter[3],
  int totalIntegrationDimension)
{
  vtkIntegrateAttributesFunctor functor(this, input, output, totalIntegrationDimension,
    fieldset_index, pdList, cdList, this->IntegrationStrategy);
  vtkSMPTools::For(0, input->GetNumberOfCells(), functor);

  const auto blockSum = functor.GetSum();
  const auto blockSumCenter = functor.GetSumCenter();
  totalSum += blockSum;
  vtkMath::Add(blockSumCenter, totalSumCenter, totalSumCenter);
}

//------------------------------------------------------------------------------
int vtkIntegrateAttributes::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  auto inputDO = vtkDataObject::GetData(inInfo);
  auto output = vtkUnstructuredGrid::GetData(outInfo);

  if (!inputDO || !output)
  {
    return 0;
  }

  // Integration of imaginary attribute with constant value 1.
  double totalSum = 0.0;
  // For computation of point/vertex location.
  double totalSumCenter[3] = { 0.0, 0.0, 0.0 };
  // For integration dimension
  int totalIntegrationDimension = 0;

  auto cdInput = vtkCompositeDataSet::SafeDownCast(inputDO);
  auto dsInput = vtkDataSet::SafeDownCast(inputDO);
  if (cdInput)
  {
    auto iter = vtk::TakeSmartPointer(cdInput->NewIterator());

    // Create the intersection field list. This is list of arrays common
    // to all blocks in the input.
    vtkIntegrateAttributesFieldList pdList;
    vtkIntegrateAttributesFieldList cdList;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        if (ds->GetNumberOfPoints() > 0)
        {
          totalIntegrationDimension =
            std::max(ds->GetMaxSpatialDimension(), totalIntegrationDimension);
          pdList.IntersectFieldList(ds->GetPointData());
          cdList.IntersectFieldList(ds->GetCellData());
        }
      }
      else if (auto dobj = iter->GetCurrentDataObject())
      {
        vtkWarningMacro("This filter cannot handle sub-datasets of type : " << dobj->GetClassName()
                                                                            << ". Skipping block");
      }
    }

    // Now initialize the output for the intersected set of arrays.
    vtkIntegrateAttributes::AllocateAttributes(pdList, output->GetPointData());
    vtkIntegrateAttributes::AllocateAttributes(cdList, output->GetCellData());

    int index = 0;
    // Now execute for each block.
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds && ds->GetNumberOfPoints() > 0)
      {
        this->ExecuteBlock(
          ds, output, index, pdList, cdList, totalSum, totalSumCenter, totalIntegrationDimension);
        index++;
      }
    }
  }
  else if (dsInput)
  {
    totalIntegrationDimension =
      std::max(dsInput->GetMaxSpatialDimension(), totalIntegrationDimension);

    // Output will have all the same attribute arrays as input, but
    // only 1 entry per array, and arrays are double.
    // Set all values to 0.  All output attributes are type double.
    vtkIntegrateAttributesFieldList pdList(1);
    vtkIntegrateAttributesFieldList cdList(1);
    pdList.InitializeFieldList(dsInput->GetPointData());
    cdList.InitializeFieldList(dsInput->GetCellData());
    this->AllocateAttributes(pdList, output->GetPointData());
    this->AllocateAttributes(cdList, output->GetCellData());
    this->ExecuteBlock(
      dsInput, output, 0, pdList, cdList, totalSum, totalSumCenter, totalIntegrationDimension);
  }
  else
  {
    vtkErrorMacro("This filter cannot handle data of type : " << inputDO->GetClassName());
    return 0;
  }

  // Here is the trick:  The satellites need a point and vertex to
  // marshal the attributes.

  // Generate point and vertex.  Add extra attributes for area too.
  // Satellites do not need the area attribute, but it does not hurt.
  double pt[3];
  // Get rid of the weight factors.
  if (totalSum != 0.0)
  {
    pt[0] = totalSumCenter[0] / totalSum;
    pt[1] = totalSumCenter[1] / totalSum;
    pt[2] = totalSumCenter[2] / totalSum;
  }
  else // totalSum == 0.0
  {
    pt[0] = totalSumCenter[0];
    pt[1] = totalSumCenter[1];
    pt[2] = totalSumCenter[2];
  }

  // Set the generated point as the only point in the output.
  vtkNew<vtkPoints> newPoints;
  newPoints->SetNumberOfPoints(1);
  newPoints->SetPoint(0, pt);
  output->SetPoints(newPoints);

  // Create a vertex cell for the generated point.
  output->Allocate(1);
  vtkIdType vertexPtIds[1];
  vertexPtIds[0] = 0;
  output->InsertNextCell(VTK_VERTEX, 1, vertexPtIds);

  // Create a new cell array for the total length, area or volume.
  vtkNew<vtkDoubleArray> sumArray;
  switch (totalIntegrationDimension)
  {
    case 1:
      sumArray->SetName("Length");
      break;
    case 2:
      sumArray->SetName("Area");
      break;
    case 3:
      sumArray->SetName("Volume");
      break;
  }
  if (totalIntegrationDimension > 0)
  {
    sumArray->SetNumberOfTuples(1);
    sumArray->SetValue(0, totalSum);
    output->GetCellData()->AddArray(sumArray);
  }

  int globalMin =
    this->PieceNodeMinToNode0(output, totalSum, totalSumCenter, totalIntegrationDimension);
  int processId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  int numProcs = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  if (globalMin == numProcs)
  {
    // there is no data in any of the processors
    if (totalSum != 0.0 && this->DivideAllCellDataByVolume)
    {
      vtkIntegrateAttributes::DivideDataArraysByConstant(output->GetCellData(), true, totalSum);
    }
    return 1;
  }
  if (processId > 0)
  {
    if (processId != globalMin)
    {
      this->SendPiece(output, totalSum, totalSumCenter, totalIntegrationDimension);
    }
  }
  else
  {
    for (int id = 1; id < numProcs; ++id)
    {
      if (id != globalMin)
      {
        this->ReceivePiece(output, id, totalSum, totalSumCenter, totalIntegrationDimension);
      }
    }

    // now that we have all of the sums from each process
    // set the point location with the global value
    if (totalSum != 0.0)
    {
      pt[0] = totalSumCenter[0] / totalSum;
      pt[1] = totalSumCenter[1] / totalSum;
      pt[2] = totalSumCenter[2] / totalSum;
      if (this->DivideAllCellDataByVolume)
      {
        vtkIntegrateAttributes::DivideDataArraysByConstant(output->GetCellData(), true, totalSum);
      }
    }
    else
    {
      pt[0] = totalSumCenter[0];
      pt[1] = totalSumCenter[1];
      pt[2] = totalSumCenter[2];
    }
    output->GetPoints()->SetPoint(0, pt);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkIntegrateAttributes::PieceNodeMinToNode0(
  vtkUnstructuredGrid* data, double& totalSum, double totalSumCenter[3], int& integrationDimension)
{
  int numProcs = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  int processId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  int localMin = (data->GetNumberOfCells() == 0 ? numProcs : processId);
  int globalMin = numProcs;
  if (numProcs == 1)
  {
    return 0;
  }
  this->Controller->AllReduce(&localMin, &globalMin, 1, vtkCommunicator::MIN_OP);
  if (globalMin == 0 || globalMin == numProcs)
  {
    return globalMin;
  }
  if (processId == 0)
  {
    this->ReceivePiece(data, globalMin, totalSum, totalSumCenter, integrationDimension);
  }
  else if (processId == globalMin)
  {
    this->SendPiece(data, totalSum, totalSumCenter, integrationDimension);
  }
  return globalMin;
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::SendPiece(vtkUnstructuredGrid* src, const double totalSum,
  const double totalSumCenter[3], const int integrationDimension)
{
  assert(this->Controller);
  double msg[5];
  msg[0] = static_cast<double>(integrationDimension);
  msg[1] = totalSum;
  msg[2] = totalSumCenter[0];
  msg[3] = totalSumCenter[1];
  msg[4] = totalSumCenter[2];
  this->Controller->Send(msg, 5, 0, vtkIntegrateAttributes::IntegrateAttrInfo);
  this->Controller->Send(src, 0, vtkIntegrateAttributes::IntegrateAttrData);
  // Done sending.  Reset src so satellites will have empty data.
  src->Initialize();
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::ReceivePiece(vtkUnstructuredGrid* mergeTo, int fromId,
  double& totalSum, double totalSumCenter[3], int& integrationDimension)
{
  assert(this->Controller);
  double msg[5];
  this->Controller->Receive(msg, 5, fromId, vtkIntegrateAttributes::IntegrateAttrInfo);
  vtkNew<vtkUnstructuredGrid> tmp;
  this->Controller->Receive(tmp, fromId, vtkIntegrateAttributes::IntegrateAttrData);
  if (vtkIntegrateAttributes::CompareIntegrationDimension(
        mergeTo, (int)(msg[0]), totalSum, totalSumCenter, integrationDimension))
  {
    totalSum += msg[1];
    totalSumCenter[0] += msg[2];
    totalSumCenter[1] += msg[3];
    totalSumCenter[2] += msg[4];
    vtkIntegrateAttributes::IntegrateSatelliteData(tmp->GetPointData(), mergeTo->GetPointData());
    vtkIntegrateAttributes::IntegrateSatelliteData(tmp->GetCellData(), mergeTo->GetCellData());
  }
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::AllocateAttributes(
  vtkIntegrateAttributesFieldList& fieldList, vtkDataSetAttributes* outda)
{
  outda->CopyAllocate(fieldList);
  vtkIntegrateAttributes::InitializeAttributes(outda);
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::InitializeAttributes(vtkDataSetAttributes* outda)
{
  for (int cc = 0, max = outda->GetNumberOfArrays(); cc < max; ++cc)
  {
    auto array = vtkDoubleArray::SafeDownCast(outda->GetAbstractArray(cc));
    assert(array != nullptr);
    array->SetNumberOfTuples(1);
    // It cannot hurt to zero the arrays here.
    array->FillValue(0.0);
  }

  for (int cc = 0; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    // this should not be necessary, however, the old version of
    // vtkIntegrateAttributes didn't mark active attributes for any arrays. We
    // preserve that behavior here. This is needed since filters like vtkGlyph3D
    // love to drop active attributes (incorrectly, in my opinion). Until we
    // resolve that, I am keeping this old behavior.
    outda->SetActiveAttribute(-1, cc);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::ZeroAttributes(vtkDataSetAttributes* outda)
{
  int numArrays, i, numComponents, j;
  vtkDataArray* outArray;
  numArrays = outda->GetNumberOfArrays();
  for (i = 0; i < numArrays; ++i)
  {
    outArray = outda->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
    {
      outArray->SetComponent(0, j, 0.0);
    }
  }
}

//------------------------------------------------------------------------------
// Used to sum arrays from all processes.
void vtkIntegrateAttributes::IntegrateSatelliteData(
  vtkDataSetAttributes* sendingProcAttributes, vtkDataSetAttributes* proc0Attributes)
{
  // if the sending processor has no data
  if (sendingProcAttributes->GetNumberOfArrays() == 0)
  {
    return;
  }

  // when processor 0 that has no data, receives data from the min
  // processor that has data
  if (proc0Attributes->GetNumberOfArrays() == 0)
  {
    proc0Attributes->DeepCopy(sendingProcAttributes);
    return;
  }

  int numArrays, i, numComponents, j;
  vtkDataArray* inArray;
  vtkDataArray* outArray;
  numArrays = proc0Attributes->GetNumberOfArrays();
  double vIn, vOut;
  for (i = 0; i < numArrays; ++i)
  {
    outArray = proc0Attributes->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    // Protect against arrays in a different order.
    const char* name = outArray->GetName();
    if (name && name[0] != '\0')
    {
      inArray = sendingProcAttributes->GetArray(name);
      if (inArray && inArray->GetNumberOfComponents() == numComponents)
      {
        // We could template for speed.
        for (j = 0; j < numComponents; ++j)
        {
          vIn = inArray->GetComponent(0, j);
          vOut = outArray->GetComponent(0, j);
          outArray->SetComponent(0, j, vOut + vIn);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkIntegrationStrategy* vtkIntegrateAttributes::GetIntegrationStrategy()
{
  return this->IntegrationStrategy;
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::DivideDataArraysByConstant(
  vtkDataSetAttributes* data, bool skipLastArray, double sum)
{
  const int offset = skipLastArray ? -1 : 0;
  for (int i = 0; i < data->GetNumberOfArrays() + offset; ++i)
  {
    vtkDataArray* arr = data->GetArray(i);
    if (arr)
    {
      for (int j = 0; j < arr->GetNumberOfComponents(); ++j)
      {
        arr->SetComponent(0, j, arr->GetComponent(0, j) / sum);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DivideAllCellDataByVolume: " << this->DivideAllCellDataByVolume << endl;
}
VTK_ABI_NAMESPACE_END
