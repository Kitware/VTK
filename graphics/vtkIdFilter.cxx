/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkIdFilter.h"

// Description:
// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkIdFilter::vtkIdFilter()
{
  this->PointIds = 1;
  this->CellIds = 1;
  this->FieldData = 0;
}

// 
// Map ids into attribute data
//
void vtkIdFilter::Execute()
{
  int id, numPts, numCells;
  vtkScalars *newScalars;
  vtkFieldData *newField;
  vtkIntArray *ptIds;
  vtkIntArray *cellIds;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating ids!");
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  // Loop over points (if requested) and generate ids
  //
  if ( this->PointIds && numPts > 0 )
    {
    ptIds = vtkIntArray::New();
    ptIds->SetNumberOfValues(numPts);

    for (id=0; id < numPts; id++)
      {
      ptIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      newScalars = vtkScalars::New();
      newScalars->SetData(ptIds);
      outPD->SetScalars(newScalars);
      newScalars->Delete();
      }
    else
      {
      newField = vtkFieldData::New();
      newField->SetNumberOfArrays(1);
      newField->SetArray(0, ptIds);
      outPD->SetFieldData(newField);
      newField->Delete();
      }
    ptIds->Delete();
    }

  // Loop over cells (if requested) and generate ids
  //
  if ( this->PointIds && numCells > 0 )
    {
    cellIds = vtkIntArray::New();
    cellIds->SetNumberOfValues(numCells);

    for (id=0; id < numCells; id++)
      {
      cellIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      newScalars = vtkScalars::New();
      newScalars->SetData(cellIds);
      outCD->SetScalars(newScalars);
      newScalars->Delete();
      }
    else
      {
      newField = vtkFieldData::New();
      newField->SetNumberOfArrays(1);
      newField->SetArray(0, cellIds);
      outCD->SetFieldData(newField);
      newField->Delete();
      }
    cellIds->Delete();
    }

  // Update self
  //
  outPD->PassNoReplaceData(inPD);
  outCD->PassNoReplaceData(inCD);
}

void vtkIdFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Point Ids: " << (this->PointIds ? "On\n" : "Off\n");
  os << indent << "Cell Ids: " << (this->CellIds ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
}
