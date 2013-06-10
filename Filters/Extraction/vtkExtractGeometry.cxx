/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractGeometry.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractGeometry);
vtkCxxSetObjectMacro(vtkExtractGeometry,ImplicitFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct object with ExtractInside turned on.
vtkExtractGeometry::vtkExtractGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
    {
    this->ImplicitFunction->Register(this);
    }

  this->ExtractInside = 1;
  this->ExtractBoundaryCells = 0;
  this->ExtractOnlyBoundaryCells = 0;
}

//----------------------------------------------------------------------------
vtkExtractGeometry::~vtkExtractGeometry()
{
  this->SetImplicitFunction(NULL);
}

// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkExtractGeometry::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkExtractGeometry::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, numPts, numCells, i, cellId, newCellId, newId, *pointMap;
  vtkIdList *cellPts;
  vtkCell *cell;
  int numCellPts;
  double x[3];
  double multiplier;
  vtkPoints *newPts;
  vtkIdList *newCellPts;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  int npts;

  vtkDebugMacro(<< "Extracting geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return 1;
    }

  // As this filter is doing a subsetting operation, set the Copy Tuple flag
  // for GlobalIds array so that, if present, it will be copied to the output.
  outputPD->CopyGlobalIdsOn();
  outputCD->CopyGlobalIdsOn();

  newCellPts = vtkIdList::New();
  newCellPts->Allocate(VTK_CELL_SIZE);

  if ( this->ExtractInside )
    {
    multiplier = 1.0;
    }
  else
    {
    multiplier = -1.0;
    }

  // Loop over all points determining whether they are inside the
  // implicit function. Copy the points and point data if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new vtkIdType[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++)
    {
    pointMap[i] = -1;
    }

  output->Allocate(numCells/4); //allocate storage for geometry/topology
  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);
  vtkFloatArray *newScalars = NULL;

  if ( ! this->ExtractBoundaryCells )
    {
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);
      if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) < 0.0 )
        {
        newId = newPts->InsertNextPoint(x);
        pointMap[ptId] = newId;
        outputPD->CopyData(pd,ptId,newId);
        }
      }
    }
  else
    {
    // To extract boundary cells, we have to create supplemental information
    double val;
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfValues(numPts);

    for (ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);
      val = this->ImplicitFunction->FunctionValue(x) * multiplier;
      newScalars->SetValue(ptId, val);
      }
    }

  // Now loop over all cells to see whether they are inside implicit
  // function (or on boundary if ExtractBoundaryCells is on).
  //
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    newCellPts->Reset();
    if ( ! this->ExtractBoundaryCells ) //requires less work
      {
      for ( npts=0, i=0; i < numCellPts; i++, npts++)
        {
        ptId = cellPts->GetId(i);
        if ( pointMap[ptId] < 0 )
          {
          break; //this cell won't be inserted
          }
        else
          {
          newCellPts->InsertId(i,pointMap[ptId]);
          }
        }
      } //if don't want to extract boundary cells

    else //want boundary cells
      {
      for ( npts=0, i=0; i < numCellPts; i++ )
        {
        ptId = cellPts->GetId(i);
        if ( newScalars->GetValue(ptId) <= 0.0 )
          {
          npts++;
          }
        }
      int extraction_condition = 0;
      if ( this->ExtractOnlyBoundaryCells )
        {
        if ( ( npts > 0 ) && ( npts != numCellPts ) )
          {
          extraction_condition = 1;
          }
        }
      else
        {
        if ( npts > 0 )
          {
          extraction_condition = 1;
          }
        }
      if ( extraction_condition )
        {
        for ( i=0; i < numCellPts; i++ )
          {
          ptId = cellPts->GetId(i);
          if ( pointMap[ptId] < 0 )
            {
            input->GetPoint(ptId, x);
            newId = newPts->InsertNextPoint(x);
            pointMap[ptId] = newId;
            outputPD->CopyData(pd,ptId,newId);
            }
          newCellPts->InsertId(i,pointMap[ptId]);
          }
        }//a boundary or interior cell
      }//if mapping boundary cells

    int extraction_condition = 0;
    if ( this->ExtractOnlyBoundaryCells )
      {
      if ( npts != numCellPts && (this->ExtractBoundaryCells && npts > 0) )
        {
        extraction_condition = 1;
        }
      }
    else
      {
      if ( npts >= numCellPts || (this->ExtractBoundaryCells && npts > 0) )
        {
        extraction_condition = 1;
        }
      }
    if ( extraction_condition )
      {
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) &&
          cell->GetCellType() == VTK_POLYHEDRON)
        {
        newCellPts->Reset();
        vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newCellPts);
        vtkUnstructuredGrid::ConvertFaceStreamPointIds(newCellPts, pointMap);
        }
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outputCD->CopyData(cd,cellId,newCellId);
      }
    }//for all cells

  // Update ourselves and release memory
  //
  delete [] pointMap;
  newCellPts->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  if ( this->ExtractBoundaryCells )
    {
    newScalars->Delete();
    }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractGeometry::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Implicit Function: "
     << static_cast<void *>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: "
     << (this->ExtractInside ? "On\n" : "Off\n");
  os << indent << "Extract Boundary Cells: "
     << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
  os << indent << "Extract Only Boundary Cells: "
     << (this->ExtractOnlyBoundaryCells ? "On\n" : "Off\n");
}
