/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPlaneSource.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkTCoords.h"
#include "vtkMath.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPlaneSource* vtkPlaneSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPlaneSource");
  if(ret)
    {
    return (vtkPlaneSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPlaneSource;
}





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
  vtkIdType pts[4];
  int i, j, ii;
  int numPts;
  int numPolys;
  vtkPoints *newPoints; 
  vtkNormals *newNormals;
  vtkTCoords *newTCoords;
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  // Check input
  for ( i=0; i < 3; i++ )
    {
    v1[i] = this->Point1[i] - this->Origin[i];
    v2[i] = this->Point2[i] - this->Origin[i];
    }
  if ( !this->UpdatePlane(v1,v2) )
    {
    return;
    }

  //
  // Set things up; allocate memory
  //
  numPts = (this->XResolution+1) * (this->YResolution+1);
  numPolys = this->XResolution * this->YResolution;

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkNormals::New();
  newNormals->Allocate(numPts);
  newTCoords = vtkTCoords::New();
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

// Set the normal to the plane. Will modify the Origin, Point1, and Point2
// instance variables as necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float N[3])
{
  float n[3], v1[3], v2[3];
  float rotVector[3], theta;
  int i;
  vtkTransform *transform = vtkTransform::New();

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
    transform->Delete();
    return;
    }
  if ( !this->UpdatePlane(v1,v2) )
    {
    transform->Delete();
    return;
    }
  
  //compute rotation vector
  vtkMath::Cross(this->Normal,n,rotVector);
  if ( vtkMath::Normalize(rotVector) == 0.0 )
    {
    transform->Delete();
    return; //no rotation
    }
  theta = acos((double)vtkMath::Dot(this->Normal,n)) / 
            vtkMath::DoubleDegreesToRadians();

  // create rotation matrix
  transform->PostMultiply();

  transform->Translate(-this->Center[0],-this->Center[1],-this->Center[2]);
  transform->RotateWXYZ(theta,rotVector[0],rotVector[1],rotVector[2]);
  transform->Translate(this->Center[0],this->Center[1],this->Center[2]);

  // transform the three defining points
  transform->TransformPoint(this->Origin,this->Origin);
  transform->TransformPoint(this->Point1,this->Point1);
  transform->TransformPoint(this->Point2,this->Point2);
    
  this->Normal[0] = n[0]; this->Normal[1] = n[1]; this->Normal[2] = n[2];

  this->Modified();
  transform->Delete();
}

// Set the normal to the plane. Will modify the Origin, Point1, and Point2
// instance variables as necessary (i.e., rotate the plane around its center).
void vtkPlaneSource::SetNormal(float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx; n[1] = ny; n[2] = nz;
  this->SetNormal(n);
}

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
    this->UpdatePlane(v1,v2);
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
    this->UpdatePlane(v1,v2);
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

// Translate the plane in the direction of the normal by the distance specified.
// Negative values move the plane in the opposite direction.
void vtkPlaneSource::Push(float distance)
{
  int i;

  if ( distance == 0.0 )
    {
    return;
    }
  for (i=0; i < 3; i++ )
    {
    this->Origin[i] += distance * this->Normal[i];
    this->Point1[i] += distance * this->Normal[i];
    this->Point2[i] += distance * this->Normal[i];
    }
  // set the new center
  for ( i=0; i < 3; i++ )
    {
    this->Center[i] = 0.5*(this->Point1[i] + this->Point2[i]);
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
