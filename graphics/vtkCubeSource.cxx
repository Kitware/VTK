/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeSource.cxx
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
#include <math.h>
#include "vtkCubeSource.hh"
#include "vtkFloatPoints.hh"
#include "vtkFloatNormals.hh"

vtkCubeSource::vtkCubeSource(float xL, float yL, float zL)
{
  this->XLength = fabs(xL);
  this->YLength = fabs(yL);
  this->ZLength = fabs(zL);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

void vtkCubeSource::Execute()
{
  float x[3], n[3];
  int numPolys=6, numPts=24;
  int i, j, k;
  int pts[4];
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Creating polygonal cube");
//
// Set things up; allocate memory
//
  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);

  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Generate points and normals
//
  numPts = 0;

  for (x[0]=Center[0]-this->XLength/2.0, n[0]=(-1.0), n[1]=n[2]=0.0, i=0; 
  i<2; i++, x[0]+=this->XLength, n[0]+=2.0)
    {
    for (x[1]=Center[1]-this->YLength/2.0, j=0; j<2; 
    j++, x[1]+=this->YLength)
      {
      for (x[2]=Center[2]-this->ZLength/2.0, k=0; k<2; 
      k++, x[2]+=this->ZLength)
        {
        newPoints->InsertNextPoint(x);
        newNormals->InsertNextNormal(n);
        }
      }
    }
  pts[0] = 0; pts[1] = 1; pts[2] = 3; pts[3] = 2; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 4; pts[1] = 6; pts[2] = 7; pts[3] = 5; 
  newPolys->InsertNextCell(4,pts);

  for (x[1]=Center[1]-this->YLength/2.0, n[1]=(-1.0), n[0]=n[2]=0.0, i=0; 
  i<2; i++, x[1]+=this->YLength, n[1]+=2.0)
    {
    for (x[0]=Center[0]-this->XLength/2.0, j=0; j<2; 
    j++, x[0]+=this->XLength)
      {
      for (x[2]=Center[2]-this->ZLength/2.0, k=0; k<2; 
      k++, x[2]+=this->ZLength)
        {
        newPoints->InsertNextPoint(x);
        newNormals->InsertNextNormal(n);
        }
      }
    }
  pts[0] = 8; pts[1] = 10; pts[2] = 11; pts[3] = 9; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 12; pts[1] = 13; pts[2] = 15; pts[3] = 14; 
  newPolys->InsertNextCell(4,pts);

  for (x[2]=Center[2]-this->ZLength/2.0, n[2]=(-1.0), n[0]=n[1]=0.0, i=0; 
  i<2; i++, x[2]+=this->ZLength, n[2]+=2.0)
    {
    for (x[1]=Center[1]-this->YLength/2.0, j=0; j<2; 
    j++, x[1]+=this->YLength)
      {
      for (x[0]=Center[0]-this->XLength/2.0, k=0; k<2; 
      k++, x[0]+=this->XLength)
        {
        newPoints->InsertNextPoint(x);
        newNormals->InsertNextNormal(n);
        }
      }
    }
  pts[0] = 16; pts[1] = 18; pts[2] = 19; pts[3] = 17; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 20; pts[1] = 21; pts[2] = 23; pts[3] = 22; 
  newPolys->InsertNextCell(4,pts);
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);
  newPolys->Delete();
}

// Description:
// Convenience method allows creation of cube by specifying bounding box.
void vtkCubeSource::SetBounds(float bounds[6])
{
  this->SetXLength(bounds[1]-bounds[0]);
  this->SetYLength(bounds[3]-bounds[2]);
  this->SetZLength(bounds[5]-bounds[4]);

  this->SetCenter((bounds[1]+bounds[0])/2.0, (bounds[3]+bounds[2])/2.0, 
                  (bounds[5]+bounds[4])/2.0);
}

void vtkCubeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "X Length: " << this->XLength << "\n";
  os << indent << "Y Length: " << this->YLength << "\n";
  os << indent << "Z Length: " << this->ZLength << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
               << this->Center[1] << ", " << this->Center[2] << ")\n";
}

