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

#include "vtkCellData.h"
#include "vtkCell.h"
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

#define VTK_MAX_CELLS_PER_POINT 4096

vtkStandardNewMacro(vtkCellDataToPointData);

namespace
{
//----------------------------------------------------------------------------
// Helper template function that implement the major part of the algorighm
// which will be expanded by the vtkTemplateMacro. The template function is
// provided so that coverage test can cover this function.
  template <typename T>
  void __spread (vtkDataSet* const src, vtkUnsignedIntArray* const num,
                 vtkDataArray* const srcarray, vtkDataArray* const dstarray,
                 vtkIdType ncells, vtkIdType npoints, vtkIdType ncomps,
                 int highestCellDimension, int contributingCellOption)
  {
    T const* const srcptr = static_cast<T const*>(srcarray->GetVoidPointer(0));
    T      * const dstptr = static_cast<T      *>(dstarray->GetVoidPointer(0));

    // zero initialization
    std::fill_n(dstptr, npoints*ncomps, T(0));

    // accumulate
    if (contributingCellOption != vtkCellDataToPointData::Patch)
    {
      T const* srcbeg = srcptr;
      for (vtkIdType cid = 0; cid < ncells; ++cid, srcbeg += ncomps)
      {
        vtkCell* cell = src->GetCell(cid);
        if (cell->GetCellDimension() >= highestCellDimension)
        {
          vtkIdList* pids = cell->GetPointIds();
          for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
          {
            T* const dstbeg = dstptr + pids->GetId(i)*ncomps;
            // accumulate cell data to point data <==> point_data += cell_data
            std::transform(srcbeg,srcbeg+ncomps,dstbeg,dstbeg,std::plus<T>());
          }
        }
      }
      // average
      T* dstbeg = dstptr;
      for (vtkIdType pid = 0; pid < npoints; ++pid, dstbeg += ncomps)
      {
        // guard against divide by zero
        if (unsigned int const denom = num->GetValue(pid))
        {
          // divide point data by the number of cells using it <==>
          // point_data /= denum
          std::transform(dstbeg, dstbeg+ncomps, dstbeg,
                         std::bind2nd(std::divides<T>(), denom));
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
          for (int comp=0;comp<ncomps;comp++)
          {
            data[comp+ncomps*cellDimension] += srcptr[comp+cellId*ncomps];
          }
        }
        for (int dimension=3;dimension>=0;dimension--)
        {
          if (numPointCells[dimension])
          {
            for (int comp=0;comp<ncomps;comp++)
            {
              dstptr[comp+pid*ncomps] = data[comp+dimension*ncomps] / numPointCells[dimension];
            }
            break;
          }
        }
      }
    }
  }

  // Special traversal algorithm for vtkUniformGrid and vtkRectilinearGrid to support blanking
  // points will not have more than 8 cells for either of these data sets
  template <typename T>
  void InterpolatePointDataWithMask(vtkCellDataToPointData* filter, T *input, vtkDataSet *output)
  {
    vtkNew<vtkIdList> allCellIds;
    allCellIds->Allocate(8);
    vtkNew<vtkIdList> cellIds;
    cellIds->Allocate(8);

    vtkIdType numPts = input->GetNumberOfPoints();

    vtkCellData *inCD = input->GetCellData();
    vtkPointData *outPD = output->GetPointData();
    outPD->InterpolateAllocate(inCD,numPts);

    double weights[8];

    int abort = 0;
    vtkIdType progressInterval = numPts / 20 + 1;
    for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
    {
      if ( !(ptId % progressInterval) )
      {
        filter->UpdateProgress(static_cast<double>(ptId)/numPts);
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

      if ( numCells > 0 )
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
  }
} // end anonymous namespace

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
  this->ContributingCellOption = vtkCellDataToPointData::All;
}

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Mapping cell data to point data");

  // Special traversal algorithm for unstructured grid
  if (input->IsA("vtkUnstructuredGrid") || input->IsA("vtkPolyData"))
  {
    return this->RequestDataForUnstructuredData(nullptr, inputVector, outputVector);
  }

  vtkDebugMacro(<<"Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->CopyGlobalIdsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  if (input->GetNumberOfPoints() < 1)
  {
    vtkDebugMacro(<<"No input point data!");
    return 1;
  }

  // Do the interpolation, taking care of masked cells if needed.
  vtkStructuredGrid *sGrid = vtkStructuredGrid::SafeDownCast(input);
  vtkUniformGrid *uniformGrid = vtkUniformGrid::SafeDownCast(input);
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    InterpolatePointDataWithMask(this, sGrid, output);
  }
  else if (uniformGrid && uniformGrid->HasAnyBlankCells())
  {
    InterpolatePointDataWithMask(this, uniformGrid, output);
  }
  else
  {
    this->InterpolatePointData(input, output);
  }

  if (!this->PassCellData)
  {
    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PassCellData: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "ContributingCellOption: " << this->ContributingCellOption << endl;
}

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestDataForUnstructuredData
(vtkInformation*,
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  vtkDataSet* const src = vtkDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* const dst = vtkDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType const ncells  = src->GetNumberOfCells ();
  vtkIdType const npoints = src->GetNumberOfPoints();
  if (ncells < 1 || npoints < 1)
  {
    vtkDebugMacro(<<"No input data!");
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
      for (vtkIdType i=0;i<src->GetNumberOfCells();i++)
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
          num->SetValue(pid, num->GetValue(pid)+1);
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

  // Copy all existing cell fields into a temporary cell data array
  vtkSmartPointer<vtkCellData> clean = vtkSmartPointer<vtkCellData>::New();
  clean->PassData(src->GetCellData());

  // Remove all fields that are not a data array.
  for (vtkIdType fid = clean->GetNumberOfArrays(); fid--;)
  {
    if (!vtkDataArray::FastDownCast(clean->GetAbstractArray(fid)))
    {
      clean->RemoveArray(fid);
    }
  }

  // Cell field list constructed from the filtered cell data array
  vtkDataSetAttributes::FieldList cfl(1);
  cfl.InitializeFieldList(clean);
  opd->InterpolateAllocate(cfl, npoints, npoints);

  for (int fid = 0, nfields = cfl.GetNumberOfFields(); fid < nfields; ++fid)
  {
    // update progress and check for an abort request.
    this->UpdateProgress((fid+1.)/nfields);
    if (this->GetAbortExecute())
    {
      break;
    }

    // indices into the field arrays associated with the cell and the point
    // respectively
    int const dstid = cfl.GetFieldIndex(fid);
    int const srcid = cfl.GetDSAIndex(0,fid);
    if  (srcid < 0 || dstid < 0)
    {
      continue;
    }

    vtkCellData * const srccelldata  = clean;
    vtkPointData* const dstpointdata = dst->GetPointData();

    if (!srccelldata || !dstpointdata)
    {
      continue;
    }

    vtkDataArray* const srcarray = srccelldata ->GetArray(srcid);
    vtkDataArray* const dstarray = dstpointdata->GetArray(dstid);
    dstarray->SetNumberOfTuples(npoints);

    vtkIdType const ncomps = srcarray->GetNumberOfComponents();
    switch (srcarray->GetDataType())
    {
      vtkTemplateMacro
        (__spread<VTK_TT>(src,num,srcarray,dstarray,ncells,npoints,ncomps, highestCellDimension, this->ContributingCellOption));
    }
  }

  if (!this->PassCellData)
  {
    dst->GetCellData()->CopyAllOff();
    dst->GetCellData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  dst->GetCellData()->PassData(src->GetCellData());

  return 1;
}

void vtkCellDataToPointData::InterpolatePointData(vtkDataSet *input, vtkDataSet *output)
{
  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkCellData *inCD = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inCD,numPts);

  double weights[VTK_MAX_CELLS_PER_POINT];

  int abort = 0;
  vtkIdType progressInterval = numPts / 20 + 1;
  for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
    }

    input->GetPointCells(ptId, cellIds);
    vtkIdType numCells = cellIds->GetNumberOfIds();

    if ( numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT )
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
}
