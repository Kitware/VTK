/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3D.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkProp3D.h"

// Construct with the following defaults: origin(0,0,0) 
// position=(0,0,0) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
vtkProp3D::vtkProp3D()
{
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  this->Orientation[0] = 0.0;
  this->Orientation[1] = 0.0;
  this->Orientation[2] = 0.0;

  this->Pickable   = 1;
  this->PickMethod = NULL;
  this->PickMethodArgDelete = NULL;
  this->PickMethodArg = NULL;
  this->Dragable   = 1;
  
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->UserMatrix = NULL;
  this->Matrix = vtkMatrix4x4::New();
  this->Transform = vtkTransform::New();
}

vtkProp3D::~vtkProp3D()
{
  if ((this->PickMethodArg)&&(this->PickMethodArgDelete))
    {
    (*this->PickMethodArgDelete)(this->PickMethodArg);
    }
  this->Matrix->Delete();
  this->Transform->Delete();
  if (this->UserMatrix)
    {
    this->UserMatrix->UnRegister(this);
    this->UserMatrix = NULL;
    }
}

// Shallow copy of an Prop3D.
vtkProp3D& vtkProp3D::operator=(const vtkProp3D& Prop3D)
{
  int i;

  for (i=0; i < 3; i++) 
    {
    this->Origin[i] = Prop3D.Origin[i];
    this->Position[i] = Prop3D.Position[i];
    this->Orientation[i] = Prop3D.Orientation[i];
    this->Center[i] = Prop3D.Center[i];
    }

  *(this->Transform) = *(Prop3D.Transform);

  this->Visibility = Prop3D.Visibility;
  this->Pickable   = Prop3D.Pickable;
  this->Dragable   = Prop3D.Dragable;
  
  for (i=0; i < 6; i++)
    {
    this->Bounds[i] = Prop3D.Bounds[i];
    }

  return *this;
}

// Incrementally change the position of the Prop3D.
void vtkProp3D::AddPosition (float deltaX,float deltaY,float deltaZ)
{
  float position[3];

  position[0] = this->Position[0] + deltaX;
  position[1] = this->Position[1] + deltaY;
  position[2] = this->Position[2] + deltaZ;
  
  this->SetPosition(position);
}

void vtkProp3D::AddPosition (float deltaPosition[3])
{
  this->AddPosition (deltaPosition[0], deltaPosition[1], deltaPosition[2]);
}

// Sets the orientation of the Prop3D.  Orientation is specified as
// X,Y and Z rotations in that order, but they are performed as
// RotateZ, RotateX, and finally RotateY.
void vtkProp3D::SetOrientation (float x,float y,float z)
{
  if (x == this->Orientation[0] && y == this->Orientation[1] 
      && z == this->Orientation[2])
    {
    return;
    }
    
  // store the coordinates
  this->Orientation[0] = x;
  this->Orientation[1] = y;
  this->Orientation[2] = z;

  vtkDebugMacro(<< " Orientation set to ( " 
                << this->Orientation[0] << ", "
                << this->Orientation[1] << ", " 
                << this->Orientation[2] << ")\n");

  this->Transform->Identity();
  this->Transform->PreMultiply ();
  this->Transform->RotateZ(this->Orientation[2]);
  this->Transform->RotateX(this->Orientation[0]);
  this->Transform->RotateY(this->Orientation[1]);

  this->Modified();
}
void vtkProp3D::SetOrientation(float a[3])
{
  this->SetOrientation(a[0],a[1],a[2]);
}

// Returns the orientation of the Prop3D as s vector of X,Y and Z rotation.
// The ordering in which these rotations must be done to generate the 
// same matrix is RotateZ, RotateX, and finally RotateY. See also 
// SetOrientation.
float *vtkProp3D::GetOrientation ()
{
  float   *orientation;

  // return the orientation of the transformation matrix
  orientation = this->Transform->GetOrientation();
  this->Orientation[0] = orientation[0];
  this->Orientation[1] = orientation[1];
  this->Orientation[2] = orientation[2];

  vtkDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");

  return this->Orientation;
} // vtkProp3D::Getorientation 

// Returns the WXYZ orientation of the Prop3D. 
float *vtkProp3D::GetOrientationWXYZ()
{
  return this->Transform->GetOrientationWXYZ();
}

// Add to the current orientation. See SetOrientation and GetOrientation for 
// more details. This basically does a GetOrientation, adds the passed in
// arguments, and then calls SetOrientation.
void vtkProp3D::AddOrientation (float a1,float a2,float a3)
{
  float *orient;

  orient = this->GetOrientation();
  this->SetOrientation(orient[0] + a1,
		       orient[1] + a2,
		       orient[2] + a3);
} 
void vtkProp3D::AddOrientation(float a[3])
{
  this->AddOrientation(a[0],a[1],a[2]);
}

// Rotate the Prop3D in degrees about the X axis using the right hand rule. The
// axis is the Prop3D's X axis, which can change as other rotations are performed.
// To rotate about the world X axis use RotateWXYZ (angle, 1, 0, 0). This rotation
// is applied before all others in the current transformation matrix.
void vtkProp3D::RotateX (float angle)
{
  this->Transform->PreMultiply ();
  this->Transform->RotateX(angle);
  this->Modified();
} 

// Rotate the Prop3D in degrees about the Y axis using the right hand rule. The
// axis is the Prop3D's Y axis, which can change as other rotations are performed.
// To rotate about the world Y axis use RotateWXYZ (angle, 0, 1, 0). This rotation
// is applied before all others in the current transformation matrix.
void vtkProp3D::RotateY (float angle)
{
  this->Transform->PreMultiply ();
  this->Transform->RotateY(angle);
  this->Modified();
} 

// Rotate the Prop3D in degrees about the Z axis using the right hand rule. The
// axis is the Prop3D's Z axis, which can change as other rotations are performed.
// To rotate about the world Z axis use RotateWXYZ (angle, 0, 0, 1). This rotation
// is applied before all others in the current transformation matrix.

void vtkProp3D::RotateZ (float angle)
{
  this->Transform->PreMultiply ();
  this->Transform->RotateZ(angle);
  this->Modified();
} 

// Rotate the Prop3D in degrees about an arbitrary axis specified by the 
// last three arguments. The axis is specified in world coordinates. To
// rotate an about its model axes, use RotateX, RotateY, RotateZ.
void vtkProp3D::RotateWXYZ (float degree, float x, float y, float z)
{
  this->Transform->PostMultiply();  
  this->Transform->RotateWXYZ(degree,x,y,z);
  this->Transform->PreMultiply();  
  this->Modified();
}

// Return a reference to the Prop3D's 4x4 composite matrix.
vtkMatrix4x4 *vtkProp3D::GetMatrixPointer()
{
  this->GetMatrix(this->Matrix);
  return this->Matrix;
} 


// Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkProp3D::GetBounds(float bounds[6])
{
  this->GetBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

// Get the center of the bounding box in world coordinates.
float *vtkProp3D::GetCenter()
{
  this->GetBounds();
  this->Center[0] = (this->Bounds[1] + this->Bounds[0])/2.0;
  this->Center[1] = (this->Bounds[3] + this->Bounds[2])/2.0;
  this->Center[2] = (this->Bounds[5] + this->Bounds[4])/2.0;
  
  return this->Center;
}

// Get the length of the diagonal of the bounding box.
float vtkProp3D::GetLength()
{
  double diff, l=0.0;
  int i;

  this->GetBounds();
  for (i=0; i<3; i++)
    {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
    }
 
  return (float)sqrt(l);
}

// Get the Prop3D's x range in world coordinates.
float *vtkProp3D::GetXRange()
{
  this->GetBounds();
  return this->Bounds;
}

// Get the Prop3D's y range in world coordinates.
float *vtkProp3D::GetYRange()
{
  this->GetBounds();
  return &(this->Bounds[2]);
}

// Get the Prop3D's z range in world coordinates.
float *vtkProp3D::GetZRange()
{
  this->GetBounds();
  return &(this->Bounds[4]);
}

// This method is invoked when an instance of vtkProp3D (or subclass, 
// e.g., vtkActor) is picked by vtkPicker.
void vtkProp3D::SetPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->PickMethod || arg != this->PickMethodArg )
    {
    // delete the current arg if there is one and a delete method
    if ((this->PickMethodArg)&&(this->PickMethodArgDelete))
      {
      (*this->PickMethodArgDelete)(this->PickMethodArg);
      }
    this->PickMethod = f;
    this->PickMethodArg = arg;
    this->Modified();
    }
}

// Set a method to delete user arguments for PickMethod.
void vtkProp3D::SetPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->PickMethodArgDelete)
    {
    this->PickMethodArgDelete = f;
    this->Modified();
    }
}

void vtkProp3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");

  if ( this->PickMethod )
    {
    os << indent << "Pick Method defined\n";
    }
  else
    {
    os << indent <<"No Pick Method\n";
    }

  if ( this->UserMatrix )
    {
    os << indent << "User Matrix: " << this->UserMatrix << "\n";
    }
  else
    {
    os << indent << "User Matrix: (none)\n";
    }

  os << indent << "Position: (" << this->Position[0] << ", " 
     << this->Position[1] << ", " << this->Position[2] << ")\n";

  os << indent << "Orientation: (" << this->Orientation[0] << ", " 
     << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " 
     << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}

