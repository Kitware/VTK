/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.cxx
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

#include "vtkProp.h"
//#include "vtkRenderWindow.h"

// Description:
// Construct with the following defaults: origin(0,0,0) 
// position=(0,0,0) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
vtkProp::vtkProp()
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

  this->Visibility = 1;
  this->Pickable   = 1;
  this->Dragable   = 1;
  
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

}

// Description:
// Shallow copy of an prop.
vtkProp& vtkProp::operator=(const vtkProp& prop)
{
  int i;

  for (i=0; i < 3; i++) 
    {
    this->Origin[i] = prop.Origin[i];
    this->Position[i] = prop.Position[i];
    this->Orientation[i] = prop.Orientation[i];
    this->Center[i] = prop.Center[i];
    }

  this->Transform = prop.Transform;

  this->Visibility = prop.Visibility;
  this->Pickable   = prop.Pickable;
  this->Dragable   = prop.Dragable;
  
  for (i=0; i < 6; i++) this->Bounds[i] = prop.Bounds[i];

  return *this;
}

// Description:
// Incrementally change the position of the prop.
void vtkProp::AddPosition (float deltaX,float deltaY,float deltaZ)
{
  float position[3];

  position[0] = this->Position[0] + deltaX;
  position[1] = this->Position[1] + deltaY;
  position[2] = this->Position[2] + deltaZ;
  
  this->SetPosition(position);
}

void vtkProp::AddPosition (float deltaPosition[3])
{
  this->AddPosition (deltaPosition[0], deltaPosition[1], deltaPosition[2]);
}

// Description:
// Sets the orientation of the prop.  Orientation is specified as
// X,Y and Z rotations in that order, but they are performed as
// RotateZ, RotateX, and finally RotateY.
void vtkProp::SetOrientation (float x,float y,float z)
{
  // store the coordinates
  this->Orientation[0] = x;
  this->Orientation[1] = y;
  this->Orientation[2] = z;

  vtkDebugMacro(<< " Orientation set to ( " 
                << this->Orientation[0] << ", "
                << this->Orientation[1] << ", " 
                << this->Orientation[2] << ")\n");

  this->Transform.Identity();
  this->Transform.PreMultiply ();
  this->Transform.RotateZ(this->Orientation[2]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateY(this->Orientation[1]);

  this->Modified();
}
void vtkProp::SetOrientation(float a[3])
{
  this->SetOrientation(a[0],a[1],a[2]);
}

// Description:
// Returns the orientation of the prop as s vector of X,Y and Z rotation.
// The ordering in which these rotations must be done to generate the 
// same matrix is RotateZ, RotateX, and finally RotateY. See also 
// SetOrientation.
float *vtkProp::GetOrientation ()
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
} // vtkProp::Getorientation 

// Description:
// Returns the WXYZ orientation of the prop. 
float *vtkProp::GetOrientationWXYZ()
{
  return this->Transform.GetOrientationWXYZ();
}

// Description:
// Add to the current orientation. See SetOrientation and GetOrientation for 
// more details. This basically does a GetOrientation, adds the passed in
// arguments, and then calls SetOrientation.
void vtkProp::AddOrientation (float a1,float a2,float a3)
{
  float *orient;

  orient = this->GetOrientation();
  this->SetOrientation(orient[0] + a1,
		       orient[1] + a2,
		       orient[2] + a3);
} 
void vtkProp::AddOrientation(float a[3])
{
  this->AddOrientation(a[0],a[1],a[2]);
}

// Description:
// Rotate the prop in degrees about the X axis using the right hand rule. The
// axis is the prop's X axis, which can change as other rotations are performed.
// To rotate about the world X axis use RotateWXYZ (angle, 1, 0, 0). This rotation
// is applied before all others in the current transformation matrix.
void vtkProp::RotateX (float angle)
{
  this->Transform.PreMultiply ();
  this->Transform.RotateX(angle);
  this->Modified();
} 

// Description:
// Rotate the prop in degrees about the Y axis using the right hand rule. The
// axis is the prop's Y axis, which can change as other rotations are performed.
// To rotate about the world Y axis use RotateWXYZ (angle, 0, 1, 0). This rotation
// is applied before all others in the current transformation matrix.
void vtkProp::RotateY (float angle)
{
  this->Transform.PreMultiply ();
  this->Transform.RotateY(angle);
  this->Modified();
} 

// Description:
// Rotate the prop in degrees about the Z axis using the right hand rule. The
// axis is the prop's Z axis, which can change as other rotations are performed.
// To rotate about the world Z axis use RotateWXYZ (angle, 0, 0, 1). This rotation
// is applied before all others in the current transformation matrix.

void vtkProp::RotateZ (float angle)
{
  this->Transform.PreMultiply ();
  this->Transform.RotateZ(angle);
  this->Modified();
} 

// Description:
// Rotate the prop in degrees about an arbitrary axis specified by the 
// last three arguments. The axis is specified in world coordinates. To
// rotate an about its model axes, use RotateX, RotateY, RotateZ.
void vtkProp::RotateWXYZ (float degree, float x, float y, float z)
{
  this->Transform.PostMultiply();  
  this->Transform.RotateWXYZ(degree,x,y,z);
  this->Transform.PreMultiply();  
  this->Modified();
}
#if 0
// Description:
// Copy the prop's composite 4x4 matrix into the matrix provided.
void vtkProp::GetMatrix(vtkMatrix4x4& result)
{
  this->GetOrientation();
  this->Transform.Push();  
  this->Transform.Identity();  
  this->Transform.PostMultiply();  

  // shift back to prop's origin
  this->Transform.Translate(-this->Origin[0],
			    -this->Origin[1],
			    -this->Origin[2]);

  // rotate
  this->Transform.RotateY(this->Orientation[1]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateZ(this->Orientation[2]);

  // move back from origin and translate
  this->Transform.Translate(this->Origin[0] + this->Position[0],
			    this->Origin[1] + this->Position[1],
			    this->Origin[2] + this->Position[2]);
   
  this->Transform.PreMultiply();  
  result = this->Transform.GetMatrix();

  this->Transform.Pop();  
} 
#endif
// Description:
// Return a reference to the prop's 4x4 composite matrix.
vtkMatrix4x4& vtkProp::GetMatrix()
{
  static vtkMatrix4x4 result;
  this->GetMatrix(result);
  return result;
} 


// Description:
// Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkProp::GetBounds(float bounds[6])
{
  this->GetBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}

// Description:
// Get the center of the bounding box in world coordinates.
float *vtkProp::GetCenter()
{
  this->GetBounds();
  this->Center[0] = (this->Bounds[1] + this->Bounds[0])/2.0;
  this->Center[1] = (this->Bounds[3] + this->Bounds[2])/2.0;
  this->Center[2] = (this->Bounds[5] + this->Bounds[4])/2.0;
  
  return this->Center;
}

// Description:
// Get the prop's x range in world coordinates.
float *vtkProp::GetXRange()
{
  this->GetBounds();
  return this->Bounds;
}

// Description:
// Get the prop's y range in world coordinates.
float *vtkProp::GetYRange()
{
  this->GetBounds();
  return &(this->Bounds[2]);
}

// Description:
// Get the prop's z range in world coordinates.
float *vtkProp::GetZRange()
{
  this->GetBounds();
  return &(this->Bounds[4]);
}

void vtkProp::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Orientation: (" << this->Orientation[0] << ", " 
     << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";
  os << indent << "Origin: (" << this->Origin[0] << ", " 
     << this->Origin[1] << ", " << this->Origin[2] << ")\n";
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");
  os << indent << "Position: (" << this->Position[0] << ", " 
     << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
}

