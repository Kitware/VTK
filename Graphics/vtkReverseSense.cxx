/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.cxx
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
#include "vtkReverseSense.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkReverseSense, "1.23");
vtkStandardNewMacro(vtkReverseSense);

// Construct object so that behavior is to reverse cell ordering and
// leave normal orientation as is.
vtkReverseSense::vtkReverseSense()
{
  this->ReverseCells = 1;
  this->ReverseNormals = 0;
}

void vtkReverseSense::Execute()
{
  vtkPolyData *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
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
  // Using MakeObject() creates normals of the same data type.
  if ( this->ReverseNormals && normals )
    {
    //first do point normals
    vtkIdType numPoints=input->GetNumberOfPoints();
    vtkDataArray *outNormals=normals->MakeObject();
    outNormals->SetNumberOfTuples(numPoints);
    float n[3];

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
    vtkDataArray *outNormals=cellNormals->MakeObject();
    outNormals->SetNumberOfTuples(numCells);
    float n[3];

    progressInterval=numCells/5+1;
    for (vtkIdType cellId=0; cellId < numCells; cellId++ )
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
}


void vtkReverseSense::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Reverse Cells: " 
     << (this->ReverseCells ? "On\n" : "Off\n");
  os << indent << "Reverse Normals: " 
     << (this->ReverseNormals ? "On\n" : "Off\n");
}

