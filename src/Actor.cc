/*=========================================================================

  Program:   Visualization Library
  Module:    Actor.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include <stdlib.h>
#include <iostream.h>
#include <math.h>

#include "Actor.hh"

// Description:
// Creates an actor with the following defaults: origin(0,0,0) 
// position(0,0,0) scale(1,1,1) visibility=1 pickable=1 dragable=1
// orientation= (0,0,0). IMPORTANT NOTE: Usually the vlRenderWindow
// method MakeActor() is used to create a device specific actor. This
// has the added benefit that a default device-specific proprty is 
// automatically created.
vlActor::vlActor()
{
  this->Mapper = NULL;
  this->Property = NULL;

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
}

vlActor::~vlActor()
{
  if ( this->Mapper ) this->Mapper->UnRegister(this);
}

// Description:
// This causes the actor to be rendered. It in turn will render the actor's
// property and then mapper.  
void vlActor::Render(vlRenderer *ren)
{
  /* render the property */
  this->Property->Render(ren);

  /* send a render to the modeller */
  this->Mapper->Render(ren);

}

void vlActor::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlActor::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    // make sure our bounds are up to date
    this->GetBounds();
    os << indent << "Bounds: (" << this->Bounds[0] << ", " 
       << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
       << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
       << this->Bounds[5] << ")\n";
    os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
    if ( this->Mapper )
      {
      os << indent << "Mapper:\n";
      this->Mapper->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Mapper: (none)\n";
      }
    os << indent << "Orientation: (" << this->Orientation[0] << ", " 
      << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";
    os << indent << "Origin: (" << this->Origin[0] << ", " 
      << this->Origin[1] << ", " << this->Origin[2] << ")\n";
    os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");
    os << indent << "Position: (" << this->Position[0] << ", " 
      << this->Position[1] << ", " << this->Position[2] << ")\n";
    if ( this->Property )
      {
      os << indent << "Property:\n";
      this->Property->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Property: (none)\n";
      }
    os << indent << "Scale: (" << this->Scale[0] << ", " 
      << this->Scale[1] << ", " << this->Scale[2] << ")\n";
    os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
    }
}

// Description:
// Sets the orientation of the actor.  Orientation is specified as
// X,Y and Z rotations in that order, but they are performed as
// RotateZ, RotateX and finally RotateY.
void vlActor::SetOrientation (float x,float y,float z)
{
  // store the coordinates
  this->Orientation[0] = x;
  this->Orientation[1] = y;
  this->Orientation[2] = z;

  vlDebugMacro(<< " Orientation set to ( " <<  this->Orientation[0] << ", "
  << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");

  this->Transform.Identity();
  this->Transform.RotateZ(this->Orientation[2]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateY(this->Orientation[1]);

  this->Modified();
}
void vlActor::SetOrientation(float a[3])
{
  this->SetOrientation(a[0],a[1],a[2]);
}

// Description:
// Returns the orientation of the actor as s vector of X,Y and Z rotation.
// The ordering in which these rotations must be done to generate the 
// same matrix is RotateZ, RotateX and finally RotateY. See also 
// SetOrientation.
float *vlActor::GetOrientation ()
{
  float   *orientation;

  // return the orientation of the transformation matrix
  orientation = this->Transform.GetOrientation();
  this->Orientation[0] = orientation[0];
  this->Orientation[1] = orientation[1];
  this->Orientation[2] = orientation[2];

  vlDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");

  return this->Orientation;
} // vlActor::Getorientation 

// Description:
// Add to the current orientation. See SetOrientation and GetOrientation for 
// more details.
void vlActor::AddOrientation (float a1,float a2,float a3)
{
  float *orient;

  orient = this->GetOrientation();
  this->SetOrientation(orient[0] + a1,
		       orient[1] + a2,
		       orient[2] + a3);
} 
void vlActor::AddOrientation(float a[3])
{
  this->AddOrientation(a[0],a[1],a[2]);
}

// Description:
// Rotate the actor in degrees about the X axis using the right hand rule.
void vlActor::RotateX (float angle)
{
  this->Transform.RotateX(angle);
  this->Modified();
} 

// Description:
// Rotate the actor in degrees about the Y axis using the right hand rule.
void vlActor::RotateY (float angle)
{
  this->Transform.RotateY(angle);
  this->Modified();
} 

// Description:
// Rotate the actor in degrees about the Z axis using the right hand rule.
void vlActor::RotateZ (float angle)
{
  this->Transform.RotateZ(angle);
  this->Modified();
} 

// Description:
// Rotate the actor in degrees about an arbitrary axis specified by the 
// last three arguments. 
void vlActor::RotateWXYZ (float degree, float x, float y, float z)
{
  this->Transform.PostMultiply();  
  this->Transform.RotateWXYZ(degree,x,y,z);
  this->Transform.PreMultiply();  
  this->Modified();
}

// Description:
// Return the 4x4 composit matrix for this actor.
vlMatrix4x4 vlActor::GetMatrix ()
{
  vlMatrix4x4 result;

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
  return(result);
} 

// Description:
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vlActor::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  
  // get the bounds of the Mapper
  bounds = this->Mapper->GetBounds();
  
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
  this->Transform.Push();  
  this->Transform.Identity();
  this->Transform.Concatenate(this->GetMatrix());

  // and transform into actors coordinates
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
  
  this->Transform.Push();  
  
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
// Get the actors x range in world coordinates.
float *vlActor::GetXRange()
{
  this->GetBounds();
  return this->Bounds;
}

// Description:
// Get the actors y range in world coordinates.
float *vlActor::GetYRange()
{
  this->GetBounds();
  return &(this->Bounds[2]);
}

// Description:
// Get the actors z range in world coordinates.
float *vlActor::GetZRange()
{
  this->GetBounds();
  return &(this->Bounds[4]);
}

