/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.cxx
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
#include "vtkShrinkFilter.hh"

vtkShrinkFilter::vtkShrinkFilter(float sf)
{
  this->ShrinkFactor = sf;
}

void vtkShrinkFilter::Execute()
{
  vtkFloatPoints *newPts;
  int i, j, cellId, numCells, numPts;
  int oldId, newId, numIds;
  float center[3], *p, pt[3];
  vtkPointData *pd, *outPD;;
  vtkIdList ptIds(VTK_CELL_SIZE), newPtIds(VTK_CELL_SIZE);
  vtkUnstructuredGrid *output=(vtkUnstructuredGrid *)this->Output;

  vtkDebugMacro(<<"Shrinking cells");

  numCells=this->Input->GetNumberOfCells();
  numPts = this->Input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
    {
    vtkErrorMacro(<<"No data to shrink!");
    return;
    }

  output->Allocate(numCells);
  newPts = new vtkFloatPoints(numPts*8,numPts);
  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts*8,numPts);
//
// Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
// then create new vertices shrunk towards center.
//
  for (cellId=0; cellId < numCells; cellId++)
    {
    this->Input->GetCellPoints(cellId,ptIds);
    numIds = ptIds.GetNumberOfIds();

    // get the center of the cell
    center[0] = center[1] = center[2] = 0.0;
    for (i=0; i < numIds; i++)
      {
      p = this->Input->GetPoint(ptIds.GetId(i));
      for (j=0; j < 3; j++) center[j] += p[j];
      }
    for (j=0; j<3; j++) center[j] /= numIds;

    // Create new points and cells
    newPtIds.Reset();
    for (i=0; i < numIds; i++)
      {
      p = this->Input->GetPoint(ptIds.GetId(i));
      for (j=0; j < 3; j++)
        pt[j] = center[j] + this->ShrinkFactor*(p[j] - center[j]);

      oldId = ptIds.GetId(i);
      newId = newPts->InsertNextPoint(pt);
      newPtIds.InsertId(i,newId);

      outPD->CopyData(pd, oldId, newId);
      }
    output->InsertNextCell(this->Input->GetCellType(cellId), newPtIds);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPts);
  output->Squeeze();

  newPts->Delete();
}

void vtkShrinkFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
