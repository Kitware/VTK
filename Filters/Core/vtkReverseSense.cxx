/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReverseSense.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkReverseSense);

// Construct object so that behavior is to reverse cell ordering and
// leave normal orientation as is.
vtkReverseSense::vtkReverseSense()
{
  this->ReverseCells = 1;
  this->ReverseNormals = 0;
}

int vtkReverseSense::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *normals=input->GetPointData()->GetNormals();
  vtkDataArray *cellNormals=input->GetCellData()->GetNormals();

  vtkDebugMacro(<<"Reversing sense of poly data");

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  //If specified, traverse all cells and reverse them
  int abort=0;
  vtkIdType progressInterval;

  if ( this->ReverseCells )
    {
    vtkIdType numCells=input->GetNumberOfCells();
    vtkCellArray *verts, *lines, *polys, *strips;

    //Instantiate necessary topology arrays
    verts = vtkCellArray::New();
    verts->DeepCopy(input->GetVerts());
    lines = vtkCellArray::New();
    lines->DeepCopy(input->GetLines());
    polys = vtkCellArray::New();
    polys->DeepCopy(input->GetPolys());
    strips = vtkCellArray::New();
    strips->DeepCopy(input->GetStrips());

    output->SetVerts(verts); verts->Delete();
    output->SetLines(lines); lines->Delete();
    output->SetPolys(polys);  polys->Delete();
    output->SetStrips(strips);  strips->Delete();

    progressInterval=numCells/10+1;
    for (vtkIdType cellId=0; cellId < numCells && !abort; cellId++ )
      {
      if ( ! (cellId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.6*cellId/numCells);
        abort = this->GetAbortExecute();
        }
      output->ReverseCell(cellId);
      }
    }

  //If specified and normals available, reverse orientation of normals.
  // Using NewInstance() creates normals of the same data type.
  if ( this->ReverseNormals && normals )
    {
    //first do point normals
    vtkIdType numPoints=input->GetNumberOfPoints();
    vtkDataArray *outNormals=normals->NewInstance();
    outNormals->SetNumberOfComponents(normals->GetNumberOfComponents());
    outNormals->SetNumberOfTuples(numPoints);
    double n[3];

    progressInterval=numPoints/5+1;
    for ( int ptId=0; ptId < numPoints; ptId++ )
      {
      if ( ! (ptId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.6 + 0.2*ptId/numPoints);
        abort = this->GetAbortExecute();
        }
      normals->GetTuple(ptId,n);
      n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2];
      outNormals->SetTuple(ptId,n);
      }

    output->GetPointData()->SetNormals(outNormals);
    outNormals->Delete();
    }

  //now do cell normals
  if ( this->ReverseNormals && cellNormals )
    {
    vtkIdType numCells=input->GetNumberOfCells();
    vtkDataArray *outNormals=cellNormals->NewInstance();
    outNormals->SetNumberOfComponents(cellNormals->GetNumberOfComponents());
    outNormals->SetNumberOfTuples(numCells);
    double n[3];

    progressInterval=numCells/5+1;
    for (vtkIdType cellId=0; cellId < numCells && !abort; cellId++ )
      {
      if ( ! (cellId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.8 + 0.2*cellId/numCells);
        abort = this->GetAbortExecute();
        }

      cellNormals->GetTuple(cellId,n);
      n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2];
      outNormals->SetTuple(cellId,n);
      }

    output->GetCellData()->SetNormals(outNormals);
    outNormals->Delete();
    }

  return 1;
}


void vtkReverseSense::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Reverse Cells: "
     << (this->ReverseCells ? "On\n" : "Off\n");
  os << indent << "Reverse Normals: "
     << (this->ReverseNormals ? "On\n" : "Off\n");
}
