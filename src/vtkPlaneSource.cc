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
#include "vtkMath.hh"

static vtkMath math;

// Description:
// Construct plane perpendicular to z-axis, resolution 1x1, width and height 1.0,
// and centered at the origin.
vtkPlaneSource::vtkPlaneSource()
{
  this->XResolution = 1;
  this->YResolution = 1;

  this->Origin[0] = this->Origin[1] = -0.5;
  this->Origin[2] = 0.0;

  this->Point1[0] = 0.5;
  this->Point1[1] = -0.5;
  this->Point1[2] = 0.0;

  this->Point2[0] = -0.5;
  this->Point2[1] = 0.5;
  this->Point2[2] = 0.0;

  this->Normal[2] = 1.0;
  this->Normal[0] = this->Normal[1] = 0.0;
}

// Description:
// Set the number of x-y subdivisions in the plane.
void vtkPlaneSource::SetResolution(const int xR, const int yR)
{
  if ( xR != this->XResolution || yR != this->YResolution )
    {
    this->XResolution = xR;
    this->YResolution = yR;

    this->XResolution = (this->XResolution > 0 ? this->XResolution : 1);
    this->YResolution = (this->YResolution > 0 ? this->YResolution : 1);

    this->Modified();
    }
}

void vtkPlaneSource::Execute()
{
  float x[3], tc[2], v1[3], v2[3];
  int pts[4];
  int i, j, ii;
  int numPts;
  int numPolys;
  vtkFloatPoints *newPoints; 
  vtkFloatNormals *newNormals;
  vtkFloatTCoords *newTCoords;
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  // Check input
  for ( i=0; i < 3; i++ )
    {
    v1[i] = this->Point1[i] - this->Origin[i];
    v2[i] = this->Point2[i] - this->Origin[i];
    }
  if ( !this->UpdateNormal(v1,v2) ) return;

  //
  // Set things up; allocate memory
  //
  numPts = (this->XResolution+1) * (this->YResolution+1);
  numPolys = this->XResolution * this->YResolution;

  newPoints = new vtkFloatPoints(numPts);
  newNormals = new vtkFloatNormals(numPts);
  newTCoords = new vtkFloatTCoords(numPts,2);

  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Generate points and point data
//
  for (numPts=0, i=0; i<(this->YResolution+1); i++)
    {
    tc[1] = (float) i / this->YResolution;
    for (j=0; j<(this->XResolution+1); j++)
      {
      tc[0] = (float) j / this->XResolution;

      for ( ii=0; ii < 3; ii++)
        {
        x[ii] = this->Origin[ii] + tc[0]*v1[ii] + tc[1]*v2[ii];
        }

      newPoints->InsertPoint(numPts,x);
      newTCoords->InsertTCoord(numPts,tc);
      newNormals->InsertNormal(numPts++,this->Normal);
      }
    }
//
// Generate polygon connectivity
//
  for (i=0; i<this->YResolution; i++)
    {
    for (j=0; j<this->XResolution; j++)
      {
      pts[0] = j + i*(this->XResolution+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + this->XResolution + 2;
      pts[3] = pts[0] + this->XResolution + 1;
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

// Description:
// Set the normal to the plane. Will modify the Origin, Point1, and Point2
// instance variables are necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float N[3])
{
  float n[3], v1[3], v2[3], center[3];
  float rotVector[3];
  int i;

  // compute plane axes
  for ( i=0; i < 3; i++)
    {
    n[i] = N[i];
    v1[i] = (this->Point1[i] - this->Origin[i]) / 2.0;
    v2[i] = (this->Point2[i] - this->Origin[i]) / 2.0;
    center[i] = this->Origin[i] + v1[i] + v2[i];
    }

  //make sure input is decent
  if ( math.Normalize(n) == 0.0 )
    {
    vtkErrorMacro(<<"Specified zero normal");
    return;
    }
  if ( !this->UpdateNormal(v1,v2) ) return;
  
  //compute rotation vector
  math.Cross(this->Normal,n,rotVector);
  if ( math.Normalize(rotVector) == 0.0 ) return; //no rotation

  // determine components of rotation around the three axes
  
}

// Description:
// Set the normal to the plane. Will modify the Origin, Point1, and Point2
// instance variables are necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx; n[1] = ny; n[2] = nz;
  this->SetNormal(n);
}

// Description:
// Translate the plane in the direction of the normal by the distance specified.
// Negative values move the plane in the opposite direction.
void vtkPlaneSource::Push(float distance)
{
  if ( distance == 0.0 ) return;

  for ( int i=0; i < 3; i++ )
    {
    this->Origin[i] += distance * this->Normal[i];
    this->Point1[i] += distance * this->Normal[i];
    this->Point2[i] += distance * this->Normal[i];
    }

  this->Modified();
}

// Protected method updates normals from two axes.
int vtkPlaneSource::UpdateNormal(float v1[3], float v2[3])
{
  math.Cross(v1,v2,this->Normal);
  if ( math.Normalize(this->Normal) == 0.0 )
    {
    vtkErrorMacro(<<"Bad plane coordinate system");
    return 0;
    }
  else
    {
    return 1;
    }
}

void vtkPlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "X Resolution: " << this->XResolution << "\n";
  os << indent << "Y Resolution: " << this->YResolution << "\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
                              << this->Normal[1] << ", "
                              << this->Normal[2] << ")\n";

}
