/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneSource.cxx
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
#include "vtkPlaneSource.h"
#include "vtkFloatPoints.h"
#include "vtkFloatNormals.h"
#include "vtkFloatTCoords.h"
#include "vtkMath.h"
#include "vtkTransform.h"


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

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
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
  if ( !this->UpdatePlane(v1,v2) ) return;

  //
  // Set things up; allocate memory
  //
  numPts = (this->XResolution+1) * (this->YResolution+1);
  numPolys = this->XResolution * this->YResolution;

  newPoints = vtkFloatPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatNormals::New();
  newNormals->Allocate(numPts);
  newTCoords = vtkFloatTCoords::New();
  newTCoords->Allocate(numPts,2);

  newPolys = vtkCellArray::New();
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
// instance variables as necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float N[3])
{
  float n[3], v1[3], v2[3], p[4];
  float rotVector[3], theta;
  int i;
  vtkTransform transform;

  // compute plane axes
  for ( i=0; i < 3; i++)
    {
    n[i] = N[i];
    v1[i] = this->Point1[i] - this->Origin[i];
    v2[i] = this->Point2[i] - this->Origin[i];
    }

  //make sure input is decent
  if ( vtkMath::Normalize(n) == 0.0 )
    {
    vtkErrorMacro(<<"Specified zero normal");
    return;
    }
  if ( !this->UpdatePlane(v1,v2) ) return;
  
  //compute rotation vector
  vtkMath::Cross(this->Normal,n,rotVector);
  if ( vtkMath::Normalize(rotVector) == 0.0 ) return; //no rotation
  theta = acos((double)vtkMath::Dot(this->Normal,n)) / vtkMath::DegreesToRadians();

  // create rotation matrix
  transform.PostMultiply();

  transform.Translate(-this->Center[0],-this->Center[1],-this->Center[2]);
  transform.RotateWXYZ(theta,rotVector[0],rotVector[1],rotVector[2]);
  transform.Translate(this->Center[0],this->Center[1],this->Center[2]);

  // transform the three defining points
  transform.SetPoint(this->Origin[0],this->Origin[1],this->Origin[2],1.0);
  transform.GetPoint(p);
  for (i=0; i < 3; i++) this->Origin[i] = p[i] / p[3];

  transform.SetPoint(this->Point1[0],this->Point1[1],this->Point1[2],1.0);
  transform.GetPoint(p);
  for (i=0; i < 3; i++) this->Point1[i] = p[i] / p[3];

  transform.SetPoint(this->Point2[0],this->Point2[1],this->Point2[2],1.0);
  transform.GetPoint(p);
  for (i=0; i < 3; i++) this->Point2[i] = p[i] / p[3];

  this->Modified();
}

// Description:
// Set the normal to the plane. Will modify the Origin, Point1, and Point2
// instance variables as necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx; n[1] = ny; n[2] = nz;
  this->SetNormal(n);
}

// Description:
// Set the center of the plane. Will modify the Origin, Point1, and Point2
// instance variables as necessary (i.e., translate the plane).
void vtkPlaneSource::SetCenter(float center[3])
{
  if ( this->Center[0] == center[0] && this->Center[1] == center[1] &&
  this->Center[2] == center[2] )
    {
    return; //no change
    }
  else
    {
    int i;
    float v1[3], v2[3];

    for ( i=0; i < 3; i++ )
      {
      v1[i] = this->Point1[i] - this->Origin[i];
      v2[i] = this->Point2[i] - this->Origin[i];
      }

    for ( i=0; i < 3; i++ )
      {
      this->Center[i] = center[i];
      this->Origin[i] = this->Center[i] - 0.5*(v1[i] + v2[i]);
      this->Point1[i] = this->Origin[i] + v1[i];
      this->Point2[i] = this->Origin[i] + v2[i];
      }
    this->Modified();
    }
}

// Description:
// Set the center of the plane. Will modify the Origin, Point1, and Point2
// instance variables as necessary (i.e., translate the plane).
void vtkPlaneSource::SetCenter(float x, float y, float z)
{
  float center[3];

  center[0] = x; center[1] = y; center[2] = z;
  this->SetCenter(center);
}


// modifies the normal and origin
void vtkPlaneSource::SetPoint1(float pnt[3])
{
  if ( this->Point1[0] == pnt[0] && this->Point1[1] == pnt[1] &&
       this->Point1[2] == pnt[2] )
    {
    return; //no change
    }
  else
    {
    int i;
    float v1[3], v2[3];

    for ( i=0; i < 3; i++ )
      {
      this->Point1[i] = pnt[i];
      v1[i] = this->Point1[i] - this->Origin[i];
      v2[i] = this->Point2[i] - this->Origin[i];
      }

    // set plane normal
    vtkMath::Cross(v1,v2,this->Normal);
    if ( vtkMath::Normalize(this->Normal) == 0.0 )
      {
      vtkErrorMacro(<<"Bad plane coordinate system");
      }
    this->Modified();
    }
}

// modifies the normal and origin
void vtkPlaneSource::SetPoint2(float pnt[3])
{
  if ( this->Point2[0] == pnt[0] && this->Point2[1] == pnt[1] &&
       this->Point2[2] == pnt[2] )
    {
    return; //no change
    }
  else
    {
    int i;
    float v1[3], v2[3];

    for ( i=0; i < 3; i++ )
      {
      this->Point2[i] = pnt[i];
      v1[i] = this->Point1[i] - this->Origin[i];
      v2[i] = this->Point2[i] - this->Origin[i];
      }
    // set plane normal
    vtkMath::Cross(v1,v2,this->Normal);
    if ( vtkMath::Normalize(this->Normal) == 0.0 )
      {
      vtkErrorMacro(<<"Bad plane coordinate system");
      }
    this->Modified();
    }
}

void vtkPlaneSource::SetPoint1(float x, float y, float z)
{
  float pnt[3];

  pnt[0] = x; pnt[1] = y; pnt[2] = z;
  this->SetPoint1(pnt);
}
void vtkPlaneSource::SetPoint2(float x, float y, float z)
{
  float pnt[3];

  pnt[0] = x; pnt[1] = y; pnt[2] = z;
  this->SetPoint2(pnt);
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

// Protected method updates normals and plane center from two axes.
int vtkPlaneSource::UpdatePlane(float v1[3], float v2[3])
{
  // set plane center
  for ( int i=0; i < 3; i++ )
    {
    this->Center[i] = this->Origin[i] + 0.5*(v1[i] + v2[i]);
    }

  // set plane normal
  vtkMath::Cross(v1,v2,this->Normal);
  if ( vtkMath::Normalize(this->Normal) == 0.0 )
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
  vtkPolyDataSource::PrintSelf(os,indent);

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

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

}
