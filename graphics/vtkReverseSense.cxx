/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkReverseSense.h"

// Description:
// Construct object so that behavior is to reverse cell ordering and
// leave normal orientation as is.
vtkReverseSense::vtkReverseSense()
{
  this->ReverseCells = 1;
  this->ReverseNormals = 0;
}

void vtkReverseSense::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkNormals *normals=input->GetPointData()->GetNormals();

  vtkDebugMacro(<<"Reversing sense of poly data");

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());

  //If specified, traverse all cells and reverse them
  if ( this->ReverseCells )
    {
    int numCells=input->GetNumberOfCells();
    vtkCellArray *verts, *lines, *polys, *strips;

    //Instantiate necessary topology arrays
    verts = new vtkCellArray(*(input->GetVerts()));
    lines = new vtkCellArray(*(input->GetLines()));
    polys = new vtkCellArray(*(input->GetPolys()));
    strips = new vtkCellArray(*(input->GetStrips()));

    output->SetVerts(verts);  
    output->SetLines(lines);  
    output->SetPolys(polys);  
    output->SetStrips(strips);  

    for ( int cellId=0; cellId < numCells; cellId++ )
      {
      output->ReverseCell(cellId);
      }
    }

  //If specified and normals available, reverse orientation of normals
  if ( this->ReverseNormals && normals )
    {
    int numPoints=input->GetNumberOfPoints();
    vtkNormals *outNormals=normals->MakeObject(numPoints);
    outNormals->SetNumberOfNormals(numPoints);
    float n[3];

    for ( int ptId=0; ptId < numPoints; ptId++ )
      {
      normals->GetNormal(ptId,n);
      n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2];
      outNormals->SetNormal(ptId,n);
      }

    output->GetPointData()->SetNormals(outNormals);
    }
}


void vtkReverseSense::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Reverse Cells: " << (this->ReverseCells ? "On\n" : "Off\n");
  os << indent << "Reverse Normals: " << (this->ReverseNormals ? "On\n" : "Off\n");
}

