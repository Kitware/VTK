/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.cxx
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
#include "vtkStructuredGridOutlineFilter.hh"

void vtkStructuredGridOutlineFilter::Execute()
{
  vtkStructuredGrid *input=(vtkStructuredGrid *)this->Input;
  vtkPoints *inPts;
  int i, j, k;
  int idx, gridIdx;
  vtkPointData *pd;
  int *dim, pts[2];
  vtkFloatPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<< "Creating structured grid outline");

  if ( (inPts=input->GetPoints()) == NULL )
    {
    vtkErrorMacro("No input!");
    return;
    }
  pd = input->GetPointData();
  dim = input->GetDimensions();
//
//  Allocate storage for lines and points
//
  newPts = new vtkFloatPoints(4*(dim[0]+dim[1]+dim[2]));
  newLines = new vtkCellArray;
  newLines->Allocate(newLines->EstimateSize(4*((dim[0]-1)+(dim[1]-1)+(dim[2]-1)),2));
//
//  Load data
//  x-data
//
  for (idx=j=0; j<4; j++) 
    {
    if ( j == 0 )
      gridIdx = 0;
    else if ( j == 1)
      gridIdx = (dim[1] - 1)*dim[0];
    else if ( j == 2)
      gridIdx = (dim[1] - 1)*dim[0] + (dim[2] - 1)*dim[0]*dim[1];
    else
      gridIdx = (dim[2] - 1)*dim[0]*dim[1];

    for (i=0; i<dim[0]; i++)
      newPts->InsertNextPoint(inPts->GetPoint(gridIdx+i));

    }
//
//  y-data
//
  for (j=0; j<4; j++) 
    {
    if ( j == 0 )
      gridIdx = 0;
    else if ( j == 1)
      gridIdx = dim[0] - 1;
    else if ( j == 2)
      gridIdx = (dim[0] - 1) + (dim[2]-1)*dim[0]*dim[1];
    else
      gridIdx = (dim[2] - 1)*dim[0]*dim[1];

    for (i=0; i<dim[1]; i++)
      newPts->InsertNextPoint(inPts->GetPoint(gridIdx+i*dim[0]));

    }
//
//  z-data
//
  idx = dim[0]*dim[1];
  for (j=0; j<4; j++) 
    {
    if ( j == 0 )
      gridIdx = 0;
    else if ( j == 1)
      gridIdx = (dim[0] - 1);
    else if ( j == 2)
      gridIdx = (dim[0] - 1) + (dim[1]-1)*dim[0];
    else
      gridIdx = (dim[1] - 1)*dim[0];
        
    for (i=0; i<dim[2]; i++)
      newPts->InsertNextPoint(inPts->GetPoint(gridIdx+i*idx));

    }
//
// Create lines. Rely on the fact that x, then y, then z points have been 
// created.
//
  idx = -1;
  for (k=0; k<3; k++) //loop over x-y-z directions
    {
    for (i=0; i<4; i++)
      {
      idx++;
      for (j=0; j<(dim[k]-1); j++) 
        {
        pts[0] = idx++;
        pts[1] = idx;
        newLines->InsertNextCell(2,pts);
        }
      }
    }
//
// Update selves and release memory
//
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}
