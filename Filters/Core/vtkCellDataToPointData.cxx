/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkCellDataToPointData.h"

#include "vtkArrayDispatch.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>
#include <functional>
#include <set>

#define VTK_MAX_CELLS_PER_POINT 4096

vtkStandardNewMacro(vtkCellDataToPointData);

namespace
{

//----------------------------------------------------------------------------
// Helper template function that implement the major part of the algorighm
// which will be expanded by the vtkTemplateMacro. The template function is
// provided so that coverage test can cover this function.
struct Spread
{
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* const srcarray, DstArrayT* const dstarray,
                  vtkDataSet* const src, vtkUnsignedIntArray* const num,
                  vtkIdType ncells, vtkIdType npoints, vtkIdType ncomps,
                  int highestCellDimension, int contributingCellOption) const
  {
    // Both arrays will have the same value type:
    using T = vtk::GetAPIType<SrcArrayT>;

    // zero initialization
    std::fill_n(vtk::DataArrayValueRange(dstarray).begin(),
                npoints*ncomps, T(0));

    const auto srcTuples = vtk::DataArrayTupleRange(srcarray);
    auto dstTuples = vtk::DataArrayTupleRange(dstarray);

    // accumulate
    if (contributingCellOption != vtkCellDataToPointData::Patch)
    {
      for (vtkIdType cid = 0; cid < ncells; ++cid)
      {
        vtkCell* cell = src->GetCell(cid);
        if (cell->GetCellDimension() >= highestCellDimension)
        {
          const auto srcTuple = srcTuples[cid];
          vtkIdList* pids = cell->GetPointIds();
          for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
          {
            const vtkIdType ptId = pids->GetId(i);
            auto dstTuple = dstTuples[ptId];
            // accumulate cell data to point data <==> point_data += cell_data
            std::transform(srcTuple.cbegin(),
                           srcTuple.cend(),
                           dstTuple.cbegin(),
                           dstTuple.begin(),
                           std::plus<T>());
          }
        }
      }
      // average
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        // guard against divide by zero
        if (unsigned int const denom = num->GetValue(pid))
        {
          // divide point data by the number of cells using it <==>
          // point_data /= denum
          auto dstTuple = dstTuples[pid];
          std::transform(dstTuple.cbegin(),
                         dstTuple.cend(),
                         dstTuple.begin(),
                         std::bind(std::divides<T>(), std::placeholders::_1, denom));
        }
      }
    }
    else
    { // compute over cell patches
      vtkNew<vtkIdList> cellsOnPoint;
      std::vector<T> data(4*ncomps);
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        std::fill(data.begin(), data.end(), 0);
        T numPointCells[4] = {0, 0, 0, 0};
        // Get all cells touching this point.
        src->GetPointCells(pid, cellsOnPoint);
        vtkIdType numPatchCells = cellsOnPoint->GetNumberOfIds();
        for (vtkIdType pc = 0; pc  < numPatchCells; pc++)
        {
          vtkIdType cellId = cellsOnPoint->GetId(pc);
          int cellDimension = src->GetCell(cellId)->GetCellDimension();
          numPointCells[cellDimension] += 1;
          const auto srcTuple = srcTuples[cellId];
          for (int comp=0;comp<ncomps;comp++)
          {
            data[comp+ncomps*cellDimension] += srcTuple[comp];
          }
        }
        auto dstTuple = dstTuples[pid];
        for (int dimension=3;dimension>=0;dimension--)
        {
          if (numPointCells[dimension])
          {
            for (int comp=0;comp<ncomps;comp++)
            {
              dstTuple[comp] = data[comp+dimension*ncomps] / numPointCells[dimension];
            }
            break;
          }
        }
      }
    }
  }
};

} // end anonymous namespace

class vtkCellDataToPointData::Internals
{
public:
  std::set<std::string> CellDataArrays;

  // Special traversal algorithm for vtkUniformGrid and vtkRectilinearGrid to support blanking
  // points will not have more than 8 cells for either of these data sets
  template <typename T>
  int InterpolatePointDataWithMask(vtkCellDataToPointData* filter, T* input, vtkDataSet* output)
  {
    vtkNew<vtkIdList> allCellIds;
    allCellIds->Allocate(8);
    vtkNew<vtkIdList> cellIds;
    cellIds->Allocate(8);

    vtkIdType numPts = input->GetNumberOfPoints();

    vtkCellData* inputInCD = input->GetCellData();
    vtkCellData* inCD;
    vtkPointData* outPD = output->GetPointData();

    if (!filter->GetProcessAllArrays())
    {
      inCD = vtkCellData::New();

      for (const auto& name : this->CellDataArrays)
      {
        vtkAbstractArray* arr = inputInCD->GetAbstractArray(name.c_str());
        if (arr == nullptr)
        {
          vtkWarningWithObjectMacro(filter, "cell data array name not found.");
          continue;
        }
        inCD->AddArray(arr);
      }
    }
    else
    {
      inCD = inputInCD;
    }

    outPD->InterpolateAllocate(inCD, numPts);

    double weights[8];

    int abort = 0;
    vtkIdType progressInterval = numPts / 20 + 1;
    for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
    {
      if (!(ptId % progressInterval))
      {
        filter->UpdateProgress(static_cast<double>(ptId) / numPts);
        abort = filter->GetAbortExecute();
      }
      input->GetPointCells(ptId, allCellIds);
      cellIds->Reset();
      // Only consider cells that are not masked:
      for (vtkIdType cId = 0; cId < allCellIds->GetNumberOfIds(); ++cId)
      {
        vtkIdType curCell = allCellIds->GetId(cId);
        if (input->IsCellVisible(curCell))
        {
          cellIds->InsertNextId(curCell);
        }
      }

      vtkIdType numCells = cellIds->GetNumberOfIds();

      if (numCells > 0)
      {
        double weight = 1.0 / numCells;
        for (vtkIdType cellId = 0; cellId < numCells; cellId++)
        {
          weights[cellId] = weight;
        }
        outPD->InterpolatePoint(inCD, ptId, cellIds, weights);
      }
      else
      {
        outPD->NullPoint(ptId);
      }
    }

    if (!filter->GetProcessAllArrays())
    {
      inCD->Delete();
    }

    return 1;
  }
};

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
  this->ContributingCellOption = vtkCellDataToPointData::All;
  this->ProcessAllArrays = true;
  this->Implementation = new Internals();
}

//----------------------------------------------------------------------------
vtkCellDataToPointData::~vtkCellDataToPointData()
{
  delete this->Implementation;
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::AddCellDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.insert(std::string(name));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::RemoveCellDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.erase(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::ClearCellDataArrays()
{
  if (!this->Implementation->CellDataArrays.empty())
  {
    this->Modified();
  }
  this->Implementation->CellDataArrays.clear();
}

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Mapping cell data to point data");

  // Special traversal algorithm for unstructured grid
  if (input->IsA("vtkUnstructuredGrid") || input->IsA("vtkPolyData"))
  {
    return this->RequestDataForUnstructuredData(nullptr, inputVector, outputVector);
  }

  vtkDebugMacro(<< "Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->CopyGlobalIdsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  if (input->GetNumberOfPoints() < 1)
  {
    vtkDebugMacro(<< "No input point data!");
    return 1;
  }

  // Do the interpolation, taking care of masked cells if needed.
  vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(input);
  vtkUniformGrid* uniformGrid = vtkUniformGrid::SafeDownCast(input);
  int result;
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, sGrid, output);
  }
  else if (uniformGrid && uniformGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, uniformGrid, output);
  }
  else
  {
    result = this->InterpolatePointData(input, output);
  }

  if (result == 0)
  {
    return 0;
  }

  if (!this->PassCellData)
  {
    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  return 1;
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassCellData: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "ContributingCellOption: " << this->ContributingCellOption << endl;
}

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestDataForUnstructuredData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* const src = vtkDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* const dst = vtkDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType const ncells = src->GetNumberOfCells();
  vtkIdType const npoints = src->GetNumberOfPoints();
  if (ncells < 1 || npoints < 1)
  {
    vtkDebugMacro(<< "No input data!");
    return 1;
  }

  // count the number of cells associated with each point. if we are doing patches
  // though we will do that later on.
  vtkSmartPointer<vtkUnsignedIntArray> num;
  int highestCellDimension = 0;
  if (this->ContributingCellOption != vtkCellDataToPointData::Patch)
  {
    num = vtkSmartPointer<vtkUnsignedIntArray>::New();
    num->SetNumberOfComponents(1);
    num->SetNumberOfTuples(npoints);
    std::fill_n(num->GetPointer(0), npoints, 0u);
    if (this->ContributingCellOption == vtkCellDataToPointData::DataSetMax)
    {
      int maxDimension = src->IsA("vtkPolyData") == 1 ? 2 : 3;
      for (vtkIdType i = 0; i < src->GetNumberOfCells(); i++)
      {
        int dim = src->GetCell(i)->GetCellDimension();
        if (dim > highestCellDimension)
        {
          highestCellDimension = dim;
          if (highestCellDimension == maxDimension)
          {
            break;
          }
        }
      }
    }
    vtkNew<vtkIdList> pids;
    for (vtkIdType cid = 0; cid < ncells; ++cid)
    {
      if (src->GetCell(cid)->GetCellDimension() >= highestCellDimension)
      {
        src->GetCellPoints(cid, pids);
        for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
        {
          vtkIdType const pid = pids->GetId(i);
          num->SetValue(pid, num->GetValue(pid) + 1);
        }
      }
    }
  }

  // First, copy the input to the output as a starting point
  dst->CopyStructure(src);
  vtkPointData* const opd = dst->GetPointData();

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  opd->CopyGlobalIdsOff();
  opd->PassData(src->GetPointData());
  opd->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  // Copy all existing cell fields into a temporary cell data array,
  // unless the SelectCellDataArrays option is active
  vtkSmartPointer<vtkCellData> processedCellData;
  if (!this->ProcessAllArrays)
  {
    processedCellData = vtkSmartPointer<vtkCellData>::New();

    vtkCellData* processedCellDataTemp = src->GetCellData();
    for (const auto& name : this->Implementation->CellDataArrays)
    {
      vtkAbstractArray* arr = processedCellDataTemp->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        vtkWarningMacro("cell data array name not found.");
        continue;
      }
      processedCellData->AddArray(arr);
    }
  }
  else
  {
    processedCellData = vtkSmartPointer<vtkCellData>(src->GetCellData());
  }

  // Remove all fields that are not a data array.
  for (vtkIdType fid = processedCellData->GetNumberOfArrays(); fid--;)
  {
    if (!vtkDataArray::FastDownCast(processedCellData->GetAbstractArray(fid)))
    {
      processedCellData->RemoveArray(fid);
    }
  }

  // Cell field list constructed from the filtered cell data array
  vtkDataSetAttributes::FieldList cfl(1);
  cfl.InitializeFieldList(processedCellData);
  opd->InterpolateAllocate(cfl, npoints, npoints);

  const auto nfields = processedCellData->GetNumberOfArrays();
  int fid = 0;
  auto f = [this, &fid, nfields, npoints, src, num, ncells, highestCellDimension](
             vtkAbstractArray* aa_srcarray, vtkAbstractArray* aa_dstarray) {
    // update progress and check for an abort request.
    this->UpdateProgress((fid + 1.0) / nfields);
    ++fid;

    if (this->GetAbortExecute())
    {
      return;
    }

    vtkDataArray* const srcarray = vtkDataArray::FastDownCast(aa_srcarray);
    vtkDataArray* const dstarray = vtkDataArray::FastDownCast(aa_dstarray);
    if (srcarray && dstarray)
    {
      dstarray->SetNumberOfTuples(npoints);
      vtkIdType const ncomps = srcarray->GetNumberOfComponents();

      Spread worker;
      using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
      if (!Dispatcher::Execute(srcarray, dstarray, worker, src, num,
                               ncells, npoints, ncomps, highestCellDimension,
                               this->ContributingCellOption))
      { // fallback for unknown arrays:
        worker(srcarray, dstarray, src, num, ncells, npoints, ncomps,
               highestCellDimension, this->ContributingCellOption);
      }
    }
  };

  if (processedCellData != nullptr && dst->GetPointData() != nullptr)
  {
    cfl.TransformData(0, processedCellData, dst->GetPointData(), f);
  }

  if (!this->PassCellData)
  {
    dst->GetCellData()->CopyAllOff();
    dst->GetCellData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  dst->GetCellData()->PassData(src->GetCellData());

  return 1;
}

int vtkCellDataToPointData::InterpolatePointData(vtkDataSet* input, vtkDataSet* output)
{
  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkCellData* inputInCD = input->GetCellData();
  vtkCellData* inCD;
  vtkPointData* outPD = output->GetPointData();

  if (!this->ProcessAllArrays)
  {
    inCD = vtkCellData::New();

    for (const auto& name : this->Implementation->CellDataArrays)
    {
      vtkAbstractArray* arr = inputInCD->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        vtkWarningMacro("cell data array name not found.");
        continue;
      }
      inCD->AddArray(arr);
    }
  }
  else
  {
    inCD = inputInCD;
  }

  outPD->InterpolateAllocate(inCD, numPts);

  double weights[VTK_MAX_CELLS_PER_POINT];

  int abort = 0;
  vtkIdType progressInterval = numPts / 20 + 1;
  for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = GetAbortExecute();
    }

    input->GetPointCells(ptId, cellIds);
    vtkIdType numCells = cellIds->GetNumberOfIds();

    if (numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT)
    {
      double weight = 1.0 / numCells;
      for (vtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        weights[cellId] = weight;
      }
      outPD->InterpolatePoint(inCD, ptId, cellIds, weights);
    }
    else
    {
      outPD->NullPoint(ptId);
    }
  }

  if (!this->ProcessAllArrays)
  {
    inCD->Delete();
  }

  return 1;
}
