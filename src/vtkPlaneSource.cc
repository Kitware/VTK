/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PlaneSrc.cc
  Language:  C++
  Date:      5/15/94
  Version:   1.12


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPlaneSource.hh"
#include "vtkFloatPoints.hh"
#include "vtkFloatNormals.hh"
#include "vtkFloatTCoords.hh"

// Description:
// Set the number of x-y subdivisions in the plane.
void vtkPlaneSource::SetResolution(const int xR, const int yR)
{
  if ( xR != this->XRes || yR != this->YRes )
  {
    this->XRes = xR;
    this->YRes = yR;

    this->XRes = (this->XRes > 0 ? this->XRes : 1);
    this->YRes = (this->YRes > 0 ? this->YRes : 1);

    this->Modified();
  }
}

void vtkPlaneSource::Execute()
{
  float x[3], tc[2], n[3], xinc, yinc;
  int pts[MAX_CELL_SIZE];
  int i, j;
  int numPts;
  int numPolys;
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  //
  // Set things up; allocate memory
  //
  output->Initialize();

  numPts = (this->XRes+1) * (this->YRes+1);
  numPolys = this->XRes * this->YRes;

  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);
  newTCoords = new vtkFloatTCoords(numPts,2);

  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Generate points and point data
//
  xinc = 1.0 / ((float)this->XRes);
  yinc = 1.0 / ((float)this->YRes);
  x[2] = 0.0; // z-value
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0;

  for (numPts=0, i=0; i<(this->YRes+1); i++)
    {
    x[1] = -0.5 + i*yinc;
    tc[1] = (float) i / this->YRes;
    for (j=0; j<(this->XRes+1); j++)
      {
      x[0] = -0.5 + j*xinc;
      tc[0] = (float) j / this->XRes;

      newPoints->InsertPoint(numPts,x);
      newTCoords->InsertTCoord(numPts,tc);
      newNormals->InsertNormal(numPts++,n);
      }
    }
//
// Generate polygons
//
  for (i=0; i<this->YRes; i++)
    {
    x[1] = tc[1] = i*yinc;
    for (j=0; j<this->XRes; j++)
      {
      pts[0] = j + i*(this->XRes+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + this->XRes + 2;
      pts[3] = pts[0] + this->XRes + 1;
      newPolys->InsertNextCell(4,pts);
      }
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkPlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Resolution: (" << this->XRes << " by " << this->YRes << ")\n";
}
