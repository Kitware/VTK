/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkVolume.h"

// Description:
// Creates a Volume with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkVolume::vtkVolume()
{
  this->Input = NULL;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  this->Orientation[0] = 0.0;
  this->Orientation[1] = 0.0;
  this->Orientation[2] = 0.0;

  this->Scale[0] = 1.0;
  this->Scale[1] = 1.0;
  this->Scale[2] = 1.0;

  this->Visibility = 1;
  this->Pickable   = 1;
  this->Dragable   = 1;

  this->LookupTable = NULL;
  this->SelfCreatedLookupTable = 0;
}

vtkVolume::~vtkVolume()
{
  if ( this->SelfCreatedLookupTable && this->LookupTable != NULL) 
    this->LookupTable->Delete();
}

// Description:
// Specify a lookup table (or transfer function) for the volume to use during
// the rendering process.
void vtkVolume::SetLookupTable(vtkLookupTable *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->SelfCreatedLookupTable ) this->LookupTable->Delete();
    this->SelfCreatedLookupTable = 0;
    this->LookupTable = lut;
    this->Modified();
    }
}

vtkLookupTable *vtkVolume::GetLookupTable()
{
  if ( this->LookupTable == NULL) this->CreateDefaultLookupTable();
  return this->LookupTable;
}

void vtkVolume::CreateDefaultLookupTable()
{
  if ( this->SelfCreatedLookupTable ) this->LookupTable->Delete();
  this->LookupTable = new vtkLookupTable;
  this->LookupTable->SetAlphaRange(0,1);
  this->SelfCreatedLookupTable = 1;
}

// Description:
// This causes the volume to be rendered. It, in turn, will build
// the volume's lookuptable (if none is defined).
void vtkVolume::Render()
{
  //
  // make sure that we've been properly initialized
  //
  if (this->Input == NULL ) 
    {
    vtkErrorMacro(<< "Volume has no input!");
    return;
    }
  else
    this->Input->Update();

  if ( this->LookupTable == NULL) this->CreateDefaultLookupTable();
  this->LookupTable->Build();
}

void vtkVolume::AddPosition (float deltaX,float deltaY,float deltaZ)
{
  float position[3];

  position[0] = this->Position[0] + deltaX;
  position[1] = this->Position[1] + deltaY;
  position[2] = this->Position[2] + deltaZ;
  
  this->SetPosition(position);
}

void vtkVolume::AddPosition (float deltaPosition[3])
{
  this->AddPosition (deltaPosition[0], deltaPosition[1], deltaPosition[2]);
}

// Description:
// Sets the orientation of the volume.  Orientation is specified as
// X,Y and Z rotations in that order, but they are performed as
// RotateZ, RotateX and finally RotateY.
void vtkVolume::SetOrientation (float x,float y,float z)
{
  // store the coordinates
  this->Orientation[0] = x;
  this->Orientation[1] = y;
  this->Orientation[2] = z;

  vtkDebugMacro(<< " Orientation set to ( " <<  this->Orientation[0] << ", "
  << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");

  this->Transform.Identity();
  this->Transform.RotateZ(this->Orientation[2]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateY(this->Orientation[1]);

  this->Modified();
}
void vtkVolume::SetOrientation(float a[3])
{
  this->SetOrientation(a[0],a[1],a[2]);
}

// Description:
// Returns the orientation of the Volume as s vector of X,Y and Z rotation.
// The ordering in which these rotations must be done to generate the 
// same matrix is RotateZ, RotateX and finally RotateY. See also 
// SetOrientation.
float *vtkVolume::GetOrientation ()
{
  float   *orientation;

  // return the orientation of the transformation matrix
  orientation = this->Transform.GetOrientation();
  this->Orientation[0] = orientation[0];
  this->Orientation[1] = orientation[1];
  this->Orientation[2] = orientation[2];

  vtkDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");

  return this->Orientation;
} // vtkVolume::Getorientation 

// Description:
// Add to the current orientation. See SetOrientation and GetOrientation for 
// more details.
void vtkVolume::AddOrientation (float a1,float a2,float a3)
{
  float *orient;

  orient = this->GetOrientation();
  this->SetOrientation(orient[0] + a1,
		       orient[1] + a2,
		       orient[2] + a3);
} 
void vtkVolume::AddOrientation(float a[3])
{
  this->AddOrientation(a[0],a[1],a[2]);
}

// Description:
// Rotate the volume in degrees about the X axis using the right hand rule.
void vtkVolume::RotateX (float angle)
{
  this->Transform.RotateX(angle);
  this->Modified();
} 

// Description:
// Rotate the volume in degrees about the Y axis using the right hand rule.
void vtkVolume::RotateY (float angle)
{
  this->Transform.RotateY(angle);
  this->Modified();
} 

// Description:
// Rotate the volume in degrees about the Z axis using the right hand rule.
void vtkVolume::RotateZ (float angle)
{
  this->Transform.RotateZ(angle);
  this->Modified();
} 

// Description:
// Rotate the volume in degrees about an arbitrary axis specified by the 
// last three arguments. 
void vtkVolume::RotateWXYZ (float degree, float x, float y, float z)
{
  this->Transform.PostMultiply();  
  this->Transform.RotateWXYZ(degree,x,y,z);
  this->Transform.PreMultiply();  
  this->Modified();
}

// Description:
// Copy the volume's composite 4x4 matrix into the matrix provided.
void vtkVolume::GetMatrix(vtkMatrix4x4& result)
{
  this->GetOrientation();
  this->Transform.Push();  
  this->Transform.Identity();  
  this->Transform.PreMultiply();  

  // first translate
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);
   
  // shift to origin
  this->Transform.Translate(this->Origin[0],
			    this->Origin[1],
			    this->Origin[2]);
   
  // rotate
  this->Transform.RotateZ(this->Orientation[2]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateY(this->Orientation[1]);

  // scale
  this->Transform.Scale(this->Scale[0],
			this->Scale[1],
			this->Scale[2]);

  // shift back from origin
  this->Transform.Translate(-this->Origin[0],
			    -this->Origin[1],
			    -this->Origin[2]);

  result = this->Transform.GetMatrix();

  this->Transform.Pop();  
} 

// Description:
// Return a reference to the volume's 4x4 composite matrix.
vtkMatrix4x4& vtkVolume::GetMatrix()
{
  static vtkMatrix4x4 result;
  this->GetMatrix(result);
  return result;
} 

// Description:
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkVolume::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  vtkMatrix4x4 matrix;
  
  // get the bounds of our input
  bounds = this->Input->GetBounds();
  
  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];
  
  // save the old transform
  this->GetMatrix(matrix);
  this->Transform.Push();  
  this->Transform.Identity();
  this->Transform.Concatenate(matrix);

  // and transform into Volumes coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++) 
    {
    this->Transform.SetPoint(fptr[0],fptr[1],fptr[2],1.0);
  
    // now store the result
    result = this->Transform.GetPoint();
    fptr[0] = result[0];
    fptr[1] = result[1];
    fptr[2] = result[2];
    fptr += 3;
    }
  
  this->Transform.Pop();  
  
  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 1.0e30;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -1.0e30;
  for (i = 0; i < 8; i++)
    {
    for (n = 0; n < 3; n++)
      {
      if (bbox[i*3+n] < this->Bounds[n*2]) this->Bounds[n*2] = bbox[i*3+n];
      if (bbox[i*3+n] > this->Bounds[n*2+1]) this->Bounds[n*2+1] = bbox[i*3+n];
      }
    }

  return this->Bounds;
}

// Description:
// Get the volume's x range in world coordinates.
float *vtkVolume::GetXRange()
{
  this->GetBounds();
  return this->Bounds;
}

// Description:
// Get the volume's y range in world coordinates.
float *vtkVolume::GetYRange()
{
  this->GetBounds();
  return &(this->Bounds[2]);
}

// Description:
// Get the volume's z range in world coordinates.
float *vtkVolume::GetZRange()
{
  this->GetBounds();
  return &(this->Bounds[4]);
}

void vtkVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  // make sure our bounds are up to date
  this->GetBounds();
  os << indent << "Bounds: (" << this->Bounds[0] << ", " 
     << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
     << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
     << this->Bounds[5] << ")\n";
  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  os << indent << "Orientation: (" << this->Orientation[0] << ", " 
    << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";
  os << indent << "Origin: (" << this->Origin[0] << ", " 
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");
  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale: (" << this->Scale[0] << ", " 
    << this->Scale[1] << ", " << this->Scale[2] << ")\n";
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
}

