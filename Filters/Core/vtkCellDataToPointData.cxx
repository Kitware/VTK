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
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <functional>

#define VTK_MAX_CELLS_PER_POINT 4096

vtkStandardNewMacro(vtkCellDataToPointData);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
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
  if (input->IsA("vtkUnstructuredGrid"))
  {
    return this->RequestDataForUnstructuredGrid(0, inputVector, outputVector);
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
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    this->interpolatePointDataWithMask(sGrid, output);
  }
  else
  {
    this->interpolatePointData(input, output);
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

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
// Helper template function that implement the major part of the algorighm
// which will be expanded by the vtkTemplateMacro. The template function is
// provided so that coverage test can cover this function.
namespace
{
  template <typename T>
  void __spread (vtkUnstructuredGrid* const src, vtkUnsignedIntArray* const num,
             vtkDataArray* const srcarray, vtkDataArray* const dstarray,
             vtkIdType ncells, vtkIdType npoints, vtkIdType ncomps)
  {
    T const* const srcptr = static_cast<T const*>(srcarray->GetVoidPointer(0));
    T      * const dstptr = static_cast<T      *>(dstarray->GetVoidPointer(0));

    // zero initialization
    std::fill_n(dstptr, npoints*ncomps, T(0));

    // accumulate
    T const* srcbeg = srcptr;
    vtkNew<vtkIdList> pids;
    for (vtkIdType cid = 0; cid < ncells; ++cid, srcbeg += ncomps)
    {
      src->GetCellPoints(cid, pids.GetPointer());
      for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
      {
        T* const dstbeg = dstptr + pids->GetId(i)*ncomps;
        // accumulate cell data to point data <==> point_data += cell_data
        std::transform(srcbeg,srcbeg+ncomps,dstbeg,dstbeg,std::plus<T>());
      }
    }

    // average
    T* dstbeg = dstptr;
    for (vtkIdType pid = 0; pid < npoints; ++pid, dstbeg += ncomps)
    {
      // guard against divide by zero
      if (unsigned int const denum = num->GetValue(pid))
      {
        // divide point data by the number of cells using it <==>
        // point_data /= denum
        std::transform(dstbeg, dstbeg+ncomps, dstbeg,
          std::bind2nd(std::divides<T>(), denum));
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestDataForUnstructuredGrid
  (vtkInformation*,
   vtkInformationVector** inputVector,
   vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* const src = vtkUnstructuredGrid::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* const dst = vtkUnstructuredGrid::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType const ncells  = src->GetNumberOfCells ();
  vtkIdType const npoints = src->GetNumberOfPoints();
  if (ncells < 1 || npoints < 1)
  {
    vtkDebugMacro(<<"No input data!");
    return 1;
  }

  // count the number of cells associated with each point
  vtkSmartPointer<vtkUnsignedIntArray> num
    = vtkSmartPointer<vtkUnsignedIntArray>::New();
  num->SetNumberOfComponents(1);
  num->SetNumberOfTuples(npoints);
  std::fill_n(num->GetPointer(0), npoints, 0u);
  vtkNew<vtkIdList> pids;
  for (vtkIdType cid = 0; cid < ncells; ++cid)
  {
    src->GetCellPoints(cid, pids.GetPointer());
    for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
    {
      vtkIdType const pid = pids->GetId(i);
      num->SetValue(pid, num->GetValue(pid)+1);
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
    if (!clean->GetAbstractArray(fid)->IsA("vtkDataArray"))
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
        (__spread<VTK_TT>(src,num,srcarray,dstarray,ncells,npoints,ncomps));
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

void vtkCellDataToPointData::interpolatePointData(vtkDataSet *input,
                                                  vtkDataSet *output)
{
  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkCellData *inCD = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inCD,numPts);

  std::vector<double> weights(VTK_MAX_CELLS_PER_POINT);

  int abort = 0;
  vtkIdType progressInterval = numPts / 20 + 1;
  for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
    }

    input->GetPointCells(ptId, cellIds.GetPointer());
    vtkIdType numCells = cellIds->GetNumberOfIds();

    if ( numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT )
    {
      double weight = 1.0 / numCells;
      for (vtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        weights[cellId] = weight;
      }
      outPD->InterpolatePoint(inCD, ptId, cellIds.GetPointer(), &weights[0]);
    }
    else
    {
      outPD->NullPoint(ptId);
    }
  }
}

void vtkCellDataToPointData::interpolatePointDataWithMask(
    vtkStructuredGrid *input, vtkDataSet *output)
{
  vtkNew<vtkIdList> allCellIds;
  allCellIds->Allocate(VTK_MAX_CELLS_PER_POINT);
  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkCellData *inCD = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inCD,numPts);

  std::vector<double> weights(VTK_MAX_CELLS_PER_POINT);

  int abort = 0;
  vtkIdType progressInterval = numPts / 20 + 1;
  for (vtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
    }

    input->GetPointCells(ptId, allCellIds.GetPointer());
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

    if ( numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT )
    {
      double weight = 1.0 / numCells;
      for (vtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        weights[cellId] = weight;
      }
      outPD->InterpolatePoint(inCD, ptId, cellIds.GetPointer(), &weights[0]);
    }
    else
    {
      outPD->NullPoint(ptId);
    }
  }
}

