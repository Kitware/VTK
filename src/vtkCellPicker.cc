/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.cc
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
#include "vtkCellPicker.hh"

vtkCellPicker::vtkCellPicker()
{
  this->CellId = -1;
  this->SubId = -1;
  for (int i=0; i<3; i++) this->PCoords[i] = 0.0;
}

void vtkCellPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                    vtkActor *assem, vtkActor *a, vtkMapper *m)
{
  int numCells;
  int cellId, i, minCellId, minSubId, subId;
  float x[3], tMin, t, pcoords[3], minXYZ[3], minPcoords[3];
  vtkCell *cell;
  vtkDataSet *input=m->GetInput();

  if ( (numCells = input->GetNumberOfCells()) < 1 ) return;
  //
  //  Intersect each cell with ray.  Keep track of one closest to 
  //  the eye (and within the clipping range).
  //
  minCellId = -1;
  minSubId = -1;
  for (tMin=VTK_LARGE_FLOAT,cellId=0; cellId<numCells; cellId++) 
    {
    cell = input->GetCell(cellId);

    if ( cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) 
    && t < tMin )
      {
      minCellId = cellId;
      minSubId = subId;
      for (i=0; i<3; i++)
        {
        minXYZ[i] = x[i];
        minPcoords[i] = pcoords[i];
        }
      tMin = t;
      }
    }
//
//  Now compare this against other actors.
//
  if ( minCellId>(-1) && tMin < this->GlobalTMin ) 
    {
    this->MarkPicked(assem, a, m, tMin, minXYZ);
    this->CellId = minCellId;
    this->SubId = minSubId;
    for (i=0; i<3; i++) this->PCoords[i] = minPcoords[i];
    vtkDebugMacro("Picked cell id= " << minCellId);
    }
}

void vtkCellPicker::Initialize()
{
  this->CellId = (-1);
  this->SubId = (-1);
  for (int i=0; i<3; i++) this->PCoords[i] = 0.0;
  this->vtkPicker::Initialize();
}

void vtkCellPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkPicker::PrintSelf(os,indent);

  os << indent << "Cell Id: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " 
     << this->PCoords[1] << ", " << this->PCoords[2] << ")\n";
}
