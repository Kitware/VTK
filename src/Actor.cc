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

vlActor::vlActor()
{
  this->Mapper = 0;
  this->Property = 0;

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

void vlActor::Render(vlRenderer *ren)
{
  /* render the property */
  this->Property->Render(ren);

  /* send a render to the modeller */
  this->Mapper->Render(ren);

}

void vlActor::SetMapper(vlMapper *m)
{
  if ( this->Mapper != m )
    {
    if ( this->Mapper ) this->Mapper->UnRegister(this);
    this->Mapper = m;
    this->Mapper->Register(this);
    this->Modified();
    }
}

vlMapper *vlActor::GetMapper()
{
  return this->Mapper;
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

void vlActor::RotateX (float angle)
{
  this->Transform.RotateX(angle);
  this->Modified();
} 

void vlActor::RotateY (float angle)
{
  this->Transform.RotateY(angle);
  this->Modified();
} 

void vlActor::RotateZ (float angle)
{
  this->Transform.RotateZ(angle);
  this->Modified();
} 

void vlActor::RotateWXYZ (float degree, float x, float y, float z)
{
  this->Transform.PostMultiply();  
  this->Transform.RotateWXYZ(degree,x,y,z);
  this->Transform.PreMultiply();  
  this->Modified();
}

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

// Get the bounds for this Actor
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
    this->Transform.SetVector(fptr[0],fptr[1],fptr[2],1.0);
  
    // now store the result
    result = this->Transform.GetVector();
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

// Get the actors x range in world coordinates
float *vlActor::GetXRange()
{
  this->GetBounds();
  return this->Bounds;
}

// Get the actors y range in world coordinates
float *vlActor::GetYRange()
{
  this->GetBounds();
  return &(this->Bounds[2]);
}

// Get the actors z range in world coordinates
float *vlActor::GetZRange()
{
  this->GetBounds();
  return &(this->Bounds[4]);
}

