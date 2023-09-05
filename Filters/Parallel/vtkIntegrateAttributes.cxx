// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIntegrateAttributes.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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

vtkCxxSetObjectMacro(vtkIntegrateAttributes, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
class vtkIntegrateAttributes::vtkFieldList : public vtkDataSetAttributes::FieldList
{
  typedef vtkDataSetAttributes::FieldList Superclass;

public:
  vtkFieldList(int numInputs = 0)
    : Superclass(numInputs)
  {
  }

protected:
  // overridden to only create vtkDoubleArray for numeric arrays.
  vtkSmartPointer<vtkAbstractArray> CreateArray(int type) const override
  {
    if (auto array = this->Superclass::CreateArray(type))
    {
      const int is_numeric = (array->IsNumeric());
      if (is_numeric)
      {
        return vtkSmartPointer<vtkAbstractArray>::Take(vtkDoubleArray::New());
      }
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
vtkIntegrateAttributes::vtkIntegrateAttributes()
  : Controller(nullptr)
  , DivideAllCellDataByVolume(false)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkIntegrateAttributes::~vtkIntegrateAttributes()
{
  this->SetController(nullptr);
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
  vtkFieldList& PointFieldList;
  vtkFieldList& CellFieldList;

  // input information
  unsigned char* Ghost;

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
    vtkFieldList& pdList, vtkFieldList& cdList)
    : Self(self)
    , Input(input)
    , Output(output)
    , TotalIntegrationDimension(totalIntegrationDimension)
    , FieldListIndex(fieldListIndex)
    , PointFieldList(pdList)
    , CellFieldList(cdList)
    , Ghost(input->GetCellGhostArray() ? input->GetCellGhostArray()->GetPointer(0) : nullptr)
  {
    if (this->Input->GetNumberOfCells() > 0)
    {
      // initialize internal data structures
      vtkNew<vtkGenericCell> cell;
      this->Input->GetCell(0, cell);
    }
  }

private:
  void IntegratePolyLine(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegratePolygon(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateTriangleStrip(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateTriangle(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double& sum, double sumCenter[3]);
  void IntegrateTetrahedron(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
    double sumCenter[3]);
  void IntegratePixel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateVoxel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateGeneral1DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateGeneral2DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);
  void IntegrateGeneral3DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3]);

  static void IntegrateData1(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, double k, vtkFieldList& fieldlist, int fieldlist_index);
  static void IntegrateData2(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, double k, vtkFieldList& fieldlist, int fieldlist_index);
  static void IntegrateData3(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double k, vtkFieldList& fieldlist,
    int fieldlist_index);
  static void IntegrateData4(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double k,
    vtkFieldList& fieldlist, int fieldlist_index);

public:
  void Initialize()
  {
    this->TLSum.Local() = 0;
    auto sumCenter = this->TLSumCenter.Local();
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
          this->IntegratePolyLine(this->Input, output, cellId, npts, pts, sum, sumCenter);
        }
        break;

        case VTK_TRIANGLE:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrateTriangle(
            this->Input, output, cellId, pts[0], pts[1], pts[2], sum, sumCenter);
        }
        break;

        case VTK_TRIANGLE_STRIP:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrateTriangleStrip(this->Input, output, cellId, npts, pts, sum, sumCenter);
        }
        break;

        case VTK_POLYGON:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegratePolygon(this->Input, output, cellId, npts, pts, sum, sumCenter);
        }
        break;

        case VTK_PIXEL:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegratePixel(this->Input, output, cellId, npts, pts, sum, sumCenter);
        }
        break;

        case VTK_QUAD:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrateTriangle(
            this->Input, output, cellId, pts[0], pts[1], pts[2], sum, sumCenter);
          this->IntegrateTriangle(
            this->Input, output, cellId, pts[0], pts[3], pts[2], sum, sumCenter);
        }
        break;

        case VTK_VOXEL:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrateVoxel(this->Input, output, cellId, npts, pts, sum, sumCenter);
        }
        break;

        case VTK_TETRA:
        {
          this->Input->GetCellPoints(cellId, npts, pts, cellPointIds);
          this->IntegrateTetrahedron(
            this->Input, output, cellId, pts[0], pts[1], pts[2], pts[3], sum, sumCenter);
        }
        break;

        default:
        {
          // We need to explicitly get the cell
          this->Input->GetCell(cellId, cell);

          cell->TriangulateIds(1, cellPointIds);
          switch (cellDim)
          {
            case 1:
              this->IntegrateGeneral1DCell(this->Input, output, cellId,
                cellPointIds->GetNumberOfIds(), cellPointIds->GetPointer(0), sum, sumCenter);
              break;
            case 2:
              this->IntegrateGeneral2DCell(this->Input, output, cellId,
                cellPointIds->GetNumberOfIds(), cellPointIds->GetPointer(0), sum, sumCenter);
              break;
            case 3:
              this->IntegrateGeneral3DCell(this->Input, output, cellId,
                cellPointIds->GetNumberOfIds(), cellPointIds->GetPointer(0), sum, sumCenter);
              break;
            default:
              vtkWarningWithObjectMacro(this->Self, "Unsupported Cell Dimension = " << cellDim);
          }
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
  int fieldset_index, vtkIntegrateAttributes::vtkFieldList& pdList,
  vtkIntegrateAttributes::vtkFieldList& cdList, double& totalSum, double totalSumCenter[3],
  int totalIntegrationDimension)
{
  vtkIntegrateAttributesFunctor functor(
    this, input, output, totalIntegrationDimension, fieldset_index, pdList, cdList);
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
    vtkFieldList pdList;
    vtkFieldList cdList;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds && ds->GetNumberOfPoints() > 0)
      {
        totalIntegrationDimension =
          std::max(ds->GetMaxSpatialDimension(), totalIntegrationDimension);

        pdList.IntersectFieldList(ds->GetPointData());
        cdList.IntersectFieldList(ds->GetCellData());
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
    vtkFieldList pdList(1);
    vtkFieldList cdList(1);
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
  vtkIntegrateAttributes::vtkFieldList& fieldList, vtkDataSetAttributes* outda)
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
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateData1(
  vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id, double k,
  vtkIntegrateAttributes::vtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, k](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray) {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double dv = vIn1;
        const double vOut = (dv * k) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateData2(
  vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id,
  double k, vtkIntegrateAttributes::vtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, k](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray) {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double dv = 0.5 * (vIn1 + vIn2);
        const double vOut = (dv * k) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateData3(
  vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id,
  vtkIdType pt3Id, double k, vtkIntegrateAttributes::vtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, k](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray) {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double dv = (vIn1 + vIn2 + vIn3) / 3.0;
        const double vOut = (dv * k) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };
  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateData4(
  vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id,
  vtkIdType pt3Id, vtkIdType pt4Id, double k, vtkIntegrateAttributes::vtkFieldList& fieldList,
  int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, pt4Id, k](
             vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray) {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double vIn4 = inArray->GetComponent(pt4Id, j);
        const double dv = (vIn1 + vIn2 + vIn3 + vIn4) * 0.25;
        const double vOut = (dv * k) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
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
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegratePolyLine(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3])
{
  double length;
  double pt1[3], pt2[3], mid[3];
  vtkIdType numLines, lineIdx;
  vtkIdType pt1Id, pt2Id;

  numLines = numPts - 1;
  for (lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    pt1Id = cellPtIds[lineIdx];
    pt2Id = cellPtIds[lineIdx + 1];
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = std::sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    sumCenter[0] += mid[0] * length;
    sumCenter[1] += mid[1] * length;
    sumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    vtkIntegrateAttributesFunctor::IntegrateData2(input->GetPointData(), output->GetPointData(),
      pt1Id, pt2Id, length, this->PointFieldList, this->FieldListIndex);
    vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(),
      cellId, length, this->CellFieldList, this->FieldListIndex);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateGeneral1DCell(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{
  // There should be an even number of points from the triangulation
  if (numPts % 2)
  {
    vtkWarningWithObjectMacro(this->Self,
      "Odd number of points(" << numPts << ")  encountered - skipping "
                              << " 1D Cell: " << cellId);
    return;
  }

  double length;
  double pt1[3], pt2[3], mid[3];

  for (vtkIdType pid = 0; pid < numPts; pid += 2)
  {
    input->GetPoint(cellPtIds[pid], pt1);
    input->GetPoint(cellPtIds[pid + 1], pt2);

    // Compute the length of the line.
    length = std::sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    sumCenter[0] += mid[0] * length;
    sumCenter[1] += mid[1] * length;
    sumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    vtkIntegrateAttributesFunctor::IntegrateData2(input->GetPointData(), output->GetPointData(),
      cellPtIds[pid], cellPtIds[pid + 1], length, this->PointFieldList, this->FieldListIndex);
    vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(),
      cellId, length, this->CellFieldList, this->FieldListIndex);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateTriangleStrip(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  numTris = numPts - 2;
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt1Id = cellPtIds[triIdx];
    pt2Id = cellPtIds[triIdx + 1];
    pt3Id = cellPtIds[triIdx + 2];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter);
  }
}

//------------------------------------------------------------------------------
// Works for convex polygons, and interpoaltion is not correct.
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegratePolygon(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3])
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  numTris = numPts - 2;
  pt1Id = cellPtIds[0];
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt2Id = cellPtIds[triIdx + 1];
    pt3Id = cellPtIds[triIdx + 2];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter);
  }
}

//------------------------------------------------------------------------------
// For axis aligned rectangular cells
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegratePixel(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType vtkNotUsed(numPts),
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{
  double pts[4][3];
  input->GetPoint(cellPtIds[0], pts[0]);
  input->GetPoint(cellPtIds[1], pts[1]);
  input->GetPoint(cellPtIds[2], pts[2]);
  input->GetPoint(cellPtIds[3], pts[3]);

  double l, w, a, mid[3];

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  l = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);

  w = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  a = std::abs(l * w);
  sum += a;
  // Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.25;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.25;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.25;
  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * a;
  sumCenter[1] += mid[1] * a;
  sumCenter[2] += mid[2] * a;

  // Now integrate the rest of the attributes.
  vtkIntegrateAttributesFunctor::IntegrateData4(input->GetPointData(), output->GetPointData(),
    cellPtIds[0], cellPtIds[1], cellPtIds[2], cellPtIds[3], a, this->PointFieldList,
    this->FieldListIndex);
  vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(), cellId,
    a, this->CellFieldList, this->FieldListIndex);
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateTriangle(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id,
  double& sum, double sumCenter[3])
{
  double pt1[3], pt2[3], pt3[3];
  double mid[3], v1[3], v2[3];
  double cross[3];
  double k;

  input->GetPoint(pt1Id, pt1);
  input->GetPoint(pt2Id, pt2);
  input->GetPoint(pt3Id, pt3);

  // Compute two legs.
  v1[0] = pt2[0] - pt1[0];
  v1[1] = pt2[1] - pt1[1];
  v1[2] = pt2[2] - pt1[2];
  v2[0] = pt3[0] - pt1[0];
  v2[1] = pt3[1] - pt1[1];
  v2[2] = pt3[2] - pt1[2];

  // Use the cross product to compute the area of the parallelogram.
  vtkMath::Cross(v1, v2, cross);
  k = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]) * 0.5;

  if (k == 0.0)
  {
    return;
  }
  sum += k;

  // Compute the middle, which is really just another attribute.
  mid[0] = (pt1[0] + pt2[0] + pt3[0]) / 3.0;
  mid[1] = (pt1[1] + pt2[1] + pt3[1]) / 3.0;
  mid[2] = (pt1[2] + pt2[2] + pt3[2]) / 3.0;
  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * k;
  sumCenter[1] += mid[1] * k;
  sumCenter[2] += mid[2] * k;

  // Now integrate the rest of the attributes.
  vtkIntegrateAttributesFunctor::IntegrateData3(input->GetPointData(), output->GetPointData(),
    pt1Id, pt2Id, pt3Id, k, this->PointFieldList, this->FieldListIndex);
  vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(), cellId,
    k, this->CellFieldList, this->FieldListIndex);
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateGeneral2DCell(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{
  // There should be a number of points that is a multiple of 3
  // from the triangulation
  if (numPts % 3)
  {
    vtkWarningWithObjectMacro(this->Self,
      "Number of points (" << numPts << ") is not divisible by 3 - skipping "
                           << " 2D Cell: " << cellId);
    return;
  }

  vtkIdType pt1Id, pt2Id, pt3Id;
  for (vtkIdType triIdx = 0; triIdx < numPts;)
  {
    pt1Id = cellPtIds[triIdx++];
    pt2Id = cellPtIds[triIdx++];
    pt3Id = cellPtIds[triIdx++];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter);
  }
}

//------------------------------------------------------------------------------
// For Tetrahedral cells
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateTetrahedron(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id,
  vtkIdType pt4Id, double& sum, double sumCenter[3])
{
  double pts[4][3];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double a[3], b[3], c[3], n[3], v, mid[3];
  // Compute the principle vectors around pt0 and the
  // centroid
  for (int i = 0; i < 3; i++)
  {
    a[i] = pts[1][i] - pts[0][i];
    b[i] = pts[2][i] - pts[0][i];
    c[i] = pts[3][i] - pts[0][i];
    mid[i] = (pts[0][i] + pts[1][i] + pts[2][i] + pts[3][i]) * 0.25;
  }

  // Calculate the volume of the tet which is 1/6 * the box product
  vtkMath::Cross(a, b, n);
  v = vtkMath::Dot(c, n) / 6.0;
  sum += v;

  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * v;
  sumCenter[1] += mid[1] * v;
  sumCenter[2] += mid[2] * v;

  // Integrate the attributes on the cell itself
  vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(), cellId,
    v, this->CellFieldList, this->FieldListIndex);

  // Integrate the attributes associated with the points
  vtkIntegrateAttributesFunctor::IntegrateData4(input->GetPointData(), output->GetPointData(),
    pt1Id, pt2Id, pt3Id, pt4Id, v, this->PointFieldList, this->FieldListIndex);
}

//------------------------------------------------------------------------------
// For axis aligned hexahedral cells
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateVoxel(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType vtkNotUsed(numPts),
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds[0];
  pt2Id = cellPtIds[1];
  pt3Id = cellPtIds[2];
  pt4Id = cellPtIds[3];
  pt5Id = cellPtIds[4];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  double l, w, h, v, mid[3];

  // Calculate the volume of the voxel
  l = pts[1][0] - pts[0][0];
  w = pts[2][1] - pts[0][1];
  h = pts[4][2] - pts[0][2];
  v = std::abs(l * w * h);
  sum += v;

  // Partially Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.125;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.125;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.125;

  // Integrate the attributes on the cell itself
  vtkIntegrateAttributesFunctor::IntegrateData1(input->GetCellData(), output->GetCellData(), cellId,
    v, this->CellFieldList, this->FieldListIndex);

  // Integrate the attributes associated with the points on the bottom face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8

  vtkIntegrateAttributesFunctor::IntegrateData4(input->GetPointData(), output->GetPointData(),
    pt1Id, pt2Id, pt3Id, pt4Id, v * 0.5, this->PointFieldList, this->FieldListIndex);

  // Now process the top face points
  pt1Id = cellPtIds[5];
  pt2Id = cellPtIds[6];
  pt3Id = cellPtIds[7];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  // Finish Computing the middle, which is really just another attribute.
  mid[0] += (pts[0][0] + pts[1][0] + pts[2][0] + pts[4][0]) * 0.125;
  mid[1] += (pts[0][1] + pts[1][1] + pts[2][1] + pts[4][1]) * 0.125;
  mid[2] += (pts[0][2] + pts[1][2] + pts[2][2] + pts[4][2]) * 0.125;

  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * v;
  sumCenter[1] += mid[1] * v;
  sumCenter[2] += mid[2] * v;

  // Integrate the attributes associated with the points on the top face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8
  vtkIntegrateAttributesFunctor::IntegrateData4(input->GetPointData(), output->GetPointData(),
    pt1Id, pt2Id, pt3Id, pt5Id, v * 0.5, this->PointFieldList, this->FieldListIndex);
}

//------------------------------------------------------------------------------
void vtkIntegrateAttributes::vtkIntegrateAttributesFunctor::IntegrateGeneral3DCell(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, double& sum, double sumCenter[3])
{

  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (numPts % 4)
  {
    vtkWarningWithObjectMacro(this->Self,
      "Number of points (" << numPts << ") is not divisiable by 4 - skipping "
                           << " 3D Cell: " << cellId);
    return;
  }

  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  for (vtkIdType tetIdx = 0; tetIdx < numPts;)
  {
    pt1Id = cellPtIds[tetIdx++];
    pt2Id = cellPtIds[tetIdx++];
    pt3Id = cellPtIds[tetIdx++];
    pt4Id = cellPtIds[tetIdx++];
    this->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id, pt4Id, sum, sumCenter);
  }
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
