/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkIdFilter, "1.17");
vtkStandardNewMacro(vtkIdFilter);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkIdFilter::vtkIdFilter()
{
  this->PointIds = 1;
  this->CellIds = 1;
  this->FieldData = 0;
  this->IdsArrayName = NULL;
  this->SetIdsArrayName("vtkIdFilter_Ids");
}

vtkIdFilter::~vtkIdFilter()
{
  if (this->IdsArrayName)
    {
    delete []IdsArrayName;
    }
}

// 
// Map ids into attribute data
//
void vtkIdFilter::Execute()
{
  vtkIdType numPts, numCells, id;
  vtkIdTypeArray *ptIds;
  vtkIdTypeArray *cellIds;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating ids!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  // Loop over points (if requested) and generate ids
  //
  if ( this->PointIds && numPts > 0 )
    {
    ptIds = vtkIdTypeArray::New();
    ptIds->SetNumberOfValues(numPts);

    for (id=0; id < numPts; id++)
      {
      ptIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      outPD->SetScalars(ptIds);
      outPD->CopyScalarsOff();
      }
    else
      {
      ptIds->SetName(this->IdsArrayName);
      outPD->AddArray(ptIds);
      outPD->CopyFieldOff(this->IdsArrayName);
      }
    ptIds->Delete();
    }

  // Loop over cells (if requested) and generate ids
  //
  if ( this->CellIds && numCells > 0 )
    {
    cellIds = vtkIdTypeArray::New();
    cellIds->SetNumberOfValues(numCells);

    for (id=0; id < numCells; id++)
      {
      cellIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      outCD->SetScalars(cellIds);
      outCD->CopyScalarsOff();
      }
    else
      {
      cellIds->SetName(this->IdsArrayName);
      outCD->AddArray(cellIds);
      outCD->CopyFieldOff(this->IdsArrayName);
      }
    cellIds->Delete();
    }

  outPD->PassData(inPD);
  outCD->PassData(inCD);
}

void vtkIdFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Ids: "    << (this->PointIds ? "On\n" : "Off\n");
  os << indent << "Cell Ids: "     << (this->CellIds ? "On\n" : "Off\n");
  os << indent << "Field Data: "   << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "IdsArrayName: " << (this->IdsArrayName ? this->IdsArrayName
       : "(none)") << "\n";
}
