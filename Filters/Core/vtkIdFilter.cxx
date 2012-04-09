/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

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
int vtkIdFilter::RequestData(
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
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells, id;
  vtkIdTypeArray *ptIds;
  vtkIdTypeArray *cellIds;
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

    ptIds->SetName(this->IdsArrayName);
    if ( ! this->FieldData )
      {
      int idx = outPD->AddArray(ptIds);
      outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
      outPD->CopyScalarsOff();
      }
    else
      {
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

    cellIds->SetName(this->IdsArrayName);
    if ( ! this->FieldData )
      {
      int idx = outCD->AddArray(cellIds);
      outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
      outCD->CopyScalarsOff();
      }
    else
      {
      outCD->AddArray(cellIds);
      outCD->CopyFieldOff(this->IdsArrayName);
      }
    cellIds->Delete();
    }

  outPD->PassData(inPD);
  outCD->PassData(inCD);

  return 1;
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
