/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereSource.cxx
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
#include <math.h>
#include "vtkSphereSource.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkMath.h"

// Description:
// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
vtkSphereSource::vtkSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->ThetaResolution = res;
  this->PhiResolution = res;
  this->StartTheta = 0.0;
  this->EndTheta = 360.0;
  this->StartPhi = 0.0;
  this->EndPhi = 180.0;
}

void vtkSphereSource::Execute()
{
  int i, j;
  int numPts, numPolys;
  vtkPoints *newPoints; 
  vtkNormals *newNormals;
  vtkCellArray *newPolys;
  float x[3], n[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  float startTheta, endTheta, startPhi, endPhi;
  int pts[3], base, numPoles=0, thetaResolution;
  vtkPolyData *output=(vtkPolyData *)this->Output;
//
// Set things up; allocate memory
//

  numPts = (this->PhiResolution - 1) * this->ThetaResolution + 2;
  // creating triangles
  numPolys = (this->PhiResolution - 1) * 2 * this->ThetaResolution;

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkNormals::New();
  newNormals->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
//
// Create sphere
//
  // Create north pole if needed
  if ( this->StartPhi <= 0.0 )
    {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] + this->Radius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = 1.0;
    newNormals->InsertNormal(numPoles,x);
    numPoles++;
    }

  // Create south pole if needed
  if ( this->EndPhi >= 180.0 )
    {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] - this->Radius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = -1.0;
    newNormals->InsertNormal(numPoles,x);
    numPoles++;
    }

  // Check data, determine increments, and convert to radians
  startTheta = (this->StartTheta < this->EndTheta ? this->StartTheta : this->EndTheta);
  startTheta *= vtkMath::Pi() / 180.0;
  endTheta = (this->EndTheta > this->StartTheta ? this->EndTheta : this->StartTheta);
  endTheta *= vtkMath::Pi() / 180.0;

  startPhi = (this->StartPhi < this->EndPhi ? this->StartPhi : this->EndPhi);
  startPhi *= vtkMath::Pi() / 180.0;
  endPhi = (this->EndPhi > this->StartPhi ? this->EndPhi : this->StartPhi);
  endPhi *= vtkMath::Pi() / 180.0;

  deltaPhi = (endPhi - startPhi) / this->PhiResolution;
  thetaResolution = (fabs(this->StartTheta - this->EndTheta) >= 360.0 ?
		     this->ThetaResolution : this->ThetaResolution - 1);
  deltaTheta = (endTheta - startTheta) / thetaResolution;

  // Create intermediate points
  for (i=0; i < this->ThetaResolution; i++)
    {
    theta = startTheta + i*deltaTheta;
    for (j=0; j < (this->PhiResolution-1); j++)
      {
      phi = startPhi + (j+1)*deltaPhi;
      radius = this->Radius * sin((double)phi);
      n[0] = radius * cos((double)theta);
      n[1] = radius * sin((double)theta);
      n[2] = this->Radius * cos((double)phi);
      x[0] = n[0] + this->Center[0];
      x[1] = n[1] + this->Center[1];
      x[2] = n[2] + this->Center[2];
      newPoints->InsertNextPoint(x);

      if ( (norm = vtkMath::Norm(n)) == 0.0 ) norm = 1.0;
      n[0] /= norm; n[1] /= norm; n[2] /= norm; 
      newNormals->InsertNextNormal(n);
      }
    }

  // Generate mesh connectivity
  if ( fabs(this->StartTheta - this->EndTheta) >= 360.0 )
    {  
    // this if statement is only here to work around a bug
    // in MSVC 5.0 (including SP 3). It occurs in the 
    // optimized build where MSVC simply doesn't evaluate
    // the if clause properly even if the math is done
    // outside the if statement.
    if (this->Debug)
      {
      cerr << "Work around Microsoft compiler bug\n";
      }
    base = (this->PhiResolution - 1) * this->ThetaResolution;
    }
  else
    {
    base = (this->PhiResolution - 1)*this->ThetaResolution + 1;
    }

  
  if ( this->StartPhi <= 0.0 ) // around north pole
    {
    for (i=0; i < thetaResolution; i++)
      {
      pts[0] = (this->PhiResolution-1)*i + numPoles;
      pts[1] = (((this->PhiResolution-1)*(i+1)) % base) + numPoles;
      pts[2] = 0;
      newPolys->InsertNextCell(3,pts);
      }
    }
  
  if ( this->EndPhi >= 180.0 ) // around south pole
    {
    for (i=0; i < thetaResolution; i++)
      {
      pts[0] = (this->PhiResolution-1)*i + this->PhiResolution - (2-numPoles);
      pts[2] = (((this->PhiResolution-1)*(i+1)) % base) + this->PhiResolution - (2-numPoles);
      pts[1] = numPoles - 1;
      newPolys->InsertNextCell(3,pts);
      }
    }

  // bands inbetween poles
  for (i=0; i < thetaResolution; i++)
    {
    for (j=0; j < (this->PhiResolution-2); j++)
      {
      pts[0] = numPoles + (this->PhiResolution-1)*i + j;
      pts[1] = pts[0] + 1;
      pts[2] = (((this->PhiResolution-1)*(i+1)+j) % base) + numPoles + 1;
      newPolys->InsertNextCell(3,pts);

      pts[1] = pts[2];
      pts[2] = pts[1] - 1;
      newPolys->InsertNextCell(3,pts);
      }
    }
//
// Update ourselves and release memeory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Theta Start: " << this->StartTheta << "\n";
  os << indent << "Phi Start: " << this->StartPhi << "\n";
  os << indent << "Theta End: " << this->EndTheta << "\n";
  os << indent << "Phi End: " << this->EndPhi << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";
}
