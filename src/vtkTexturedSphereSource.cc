/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedSphereSource.cc
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
#include "vtkTexturedSphereSource.hh"
#include "vtkFloatPoints.hh"
#include "vtkFloatNormals.hh"
#include "vtkMath.hh"

// Description:
// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions.
vtkTexturedSphereSource::vtkTexturedSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->ThetaResolution = res;
  this->PhiResolution = res;
}

void vtkTexturedSphereSource::Execute()
{
  int i, j;
  int numPts;
  int numPolys;
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
  vtkCellArray *newPolys;
  float x[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  int pts[3];
  vtkPolyData *output=(vtkPolyData *)this->Output;
  float tc[2];
  
  //
  // Set things up; allocate memory
  //

  numPts = (this->PhiResolution + 1) * (this->ThetaResolution + 1);
  // creating triangles
  numPolys = this->PhiResolution * 2 * this->ThetaResolution;

  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);
  newTCoords = new vtkFloatTCoords(numPts,2);
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,3));
  //
  // Create sphere
  //
  // Create intermediate points
  deltaPhi = vtkMath::Pi() / this->PhiResolution;
  deltaTheta = 2.0 * vtkMath::Pi() / this->ThetaResolution;
  for (i=0; i <= this->ThetaResolution; i++)
    {
    theta = i * deltaTheta;
    tc[0] = theta/(2.0*3.1415926);
    for (j=0; j <= this->PhiResolution; j++)
      {
      phi = j * deltaPhi;
      radius = this->Radius * sin((double)phi);
      x[0] = radius * cos((double)theta);
      x[1] = radius * sin((double)theta);
      x[2] = this->Radius * cos((double)phi);
      newPoints->InsertNextPoint(x);

      if ( (norm = vtkMath::Norm(x)) == 0.0 ) norm = 1.0;
      x[0] /= norm; x[1] /= norm; x[2] /= norm; 
      newNormals->InsertNextNormal(x);

      tc[1] = 1.0 - phi/3.1415926;
      newTCoords->InsertNextTCoord(tc);
      }
    }
  //
  // Generate mesh connectivity
  //
  // bands inbetween poles
  for (i=0; i < this->ThetaResolution; i++)
    {
    for (j=0; j < this->PhiResolution; j++)
      {
      pts[0] = (this->PhiResolution+1)*i + j;
      pts[1] = pts[0] + 1;
      pts[2] = ((this->PhiResolution+1)*(i+1)+j) + 1;
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

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkTexturedSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
}
