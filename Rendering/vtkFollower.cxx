/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMath.h"
#include "vtkFollower.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

#include <stdlib.h>
#include <math.h>

vtkCxxRevisionMacro(vtkFollower, "1.39");
vtkStandardNewMacro(vtkFollower);

// Creates a follower with no camera set
vtkFollower::vtkFollower()
{
  this->Camera = NULL;
  this->Device = vtkActor::New();
}

vtkFollower::~vtkFollower()
{
  if (this->Camera)
    {
    this->Camera->UnRegister(this);
    }
  
  this->Device->Delete();
}

// Copy the follower's composite 4x4 matrix into the matrix provided.
void vtkFollower::GetMatrix(vtkMatrix4x4 *result)
{
  double *pos, *vup;
  double Rx[3], Ry[3], Rz[3], p1[3];
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  int i;
  double distance;
  
  this->GetOrientation();
  this->Transform->Push();  
  this->Transform->PostMultiply();  
  this->Transform->Identity();

  // apply user defined matrix last if there is one 
  if (this->UserMatrix)
    {
    this->Transform->Concatenate(this->UserMatrix);
    }

  this->Transform->Translate(-this->Origin[0],
                             -this->Origin[1],
                             -this->Origin[2]);
  // scale
  this->Transform->Scale(this->Scale[0],
                         this->Scale[1],
                         this->Scale[2]);
  
  // rotate
  this->Transform->RotateY(this->Orientation[1]);
  this->Transform->RotateX(this->Orientation[0]);
  this->Transform->RotateZ(this->Orientation[2]);

  if (this->Camera)
    {
    // do the rotation
    // first rotate y 
    pos = this->Camera->GetPosition();
    vup = this->Camera->GetViewUp();

    if (this->Camera->GetParallelProjection())
      {
      this->Camera->GetDirectionOfProjection(Rz);
      Rz[0] = -Rz[0];
      Rz[1] = -Rz[1];
      Rz[2] = -Rz[2];
      }
    else
      {
      distance = sqrt(
        (pos[0] - this->Position[0])*(pos[0] - this->Position[0]) +
        (pos[1] - this->Position[1])*(pos[1] - this->Position[1]) +
        (pos[2] - this->Position[2])*(pos[2] - this->Position[2]));
      for (i = 0; i < 3; i++)
        {
        Rz[i] = (pos[i] - this->Position[i])/distance;
        }
      }
  
    vtkMath::Cross(vup,Rz,Rx);
    vtkMath::Normalize(Rx);
    vtkMath::Cross(Rz,Rx,Ry);
    
    matrix->Element[0][0] = Rx[0];
    matrix->Element[1][0] = Rx[1];
    matrix->Element[2][0] = Rx[2];
    matrix->Element[0][1] = Ry[0];
    matrix->Element[1][1] = Ry[1];
    matrix->Element[2][1] = Ry[2];
    matrix->Element[0][2] = Rz[0];
    matrix->Element[1][2] = Rz[1];
    matrix->Element[2][2] = Rz[2];
    
    this->Transform->Concatenate(matrix);
    }
  
  // translate to projection reference point PRP
  // this is the camera's position blasted through
  // the current matrix
  p1[0] = this->Origin[0] + this->Position[0];
  p1[1] = this->Origin[1] + this->Position[1];
  p1[2] = this->Origin[2] + this->Position[2];

  this->Transform->Translate(p1[0],p1[1],p1[2]);
  this->Transform->GetMatrix(result);
  
  matrix->Delete();
  this->Transform->Pop();  
} 

void vtkFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }
}

int vtkFollower::RenderOpaqueGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

int vtkFollower::RenderTranslucentGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (!this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.  
void vtkFollower::Render(vtkRenderer *ren)
{
  this->Property->Render(this, ren);
  
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  
  this->Device->SetProperty (this->Property);
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }

  /* render the texture */
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
    
  // make sure the device has the same matrix
  this->GetMatrix(matrix);
  this->Device->SetUserMatrix(matrix);
  
  this->Device->Render(ren,this->Mapper);

  matrix->Delete();
}

void vtkFollower::ShallowCopy(vtkProp *prop)
{
  vtkFollower *f = vtkFollower::SafeDownCast(prop);
  if ( f != NULL )
    {
    this->SetCamera(f->GetCamera());
    }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}



