/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.cxx
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
#include "vtkMath.h"
#include "vtkFollower.h"
#include "vtkCamera.h"

// Description:
// Creates a follower with no camera set
vtkFollower::vtkFollower()
{
  this->Camera = NULL;
  this->Device = vtkActor::New();
}

vtkFollower::~vtkFollower()
{
  this->Device->Delete();
}

// Description:
// Copy the follower's composite 4x4 matrix into the matrix provided.
void vtkFollower::GetMatrix(vtkMatrix4x4& result)
{
  float *pos;
  vtkMatrix4x4 matrix;

  this->GetOrientation();
  this->Transform.Push();  
  this->Transform.Identity();  
  this->Transform.PreMultiply();  

  // apply user defined matrix last if there is one 
  if (this->UserMatrix)
    {
    this->Transform.Concatenate(*this->UserMatrix);
    }

  // first translate
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);
   
  // shift to origin
  this->Transform.Translate(this->Origin[0],
			    this->Origin[1],
			    this->Origin[2]);
   

  // add the rotation to follow the camera
  if (this->Camera)
    {
    float distance, distance_old;
    float *vup;
    float twist = 0;
    float v1[3], v2[3], y_axis[3];
    double theta, dot, mag;
    double cosang;
    float vn[3];

    // calc the direction
    pos = this->Camera->GetPosition();
    vup = this->Camera->GetViewUp();

    // first rotate y 
    distance = sqrt((this->Position[0]-pos[0])*(this->Position[0]-pos[0]) +
		    (this->Position[2]-pos[2])*(this->Position[2]-pos[2]));
    vn[0] = (pos[0] - this->Position[0])/distance;
    vn[1] = (pos[1] - this->Position[1])/distance;
    vn[2] = (pos[2] - this->Position[2])/distance;

    // rotate y
    if (distance > 0.0)
      {
      matrix[0][0] = (this->Position[2]-pos[2])/distance;
      matrix[0][2] = -1.0*(pos[0] - this->Position[0])/distance;
      }
    else
      {
      if (this->Position[1] < pos[1])
	{
	matrix[0][0] = -1.0;
	}
      else
	{
	matrix[0][0] = 1.0;
	}
      matrix[0][2] = 0.0;
      }
    matrix[0][1] = matrix[0][3] = 0.0;
    matrix[1][1] = 1.0;
    matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0;
    matrix[2][0] = -1.0*matrix[0][2];
    matrix[2][2] = matrix[0][0];
    matrix[2][3] = 0.0;
    matrix[2][1] = 0.0;
    matrix[3][3] = 1.0;
    matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
    this->Transform.Concatenate(matrix);

    // now rotate x 
    distance_old = distance;
    distance = sqrt((this->Position[0]-pos[0])*(this->Position[0]-pos[0]) +
		    (this->Position[1]-pos[1])*(this->Position[1]-pos[1]) +
		    (this->Position[2]-pos[2])*(this->Position[2]-pos[2]));
    matrix[0][0] = 1.0;
    matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0;
    matrix[1][1] = distance_old/distance;
    matrix[1][2] = (this->Position[1] - pos[1])/distance;
    matrix[1][0] = matrix[1][3] = 0.0;
    matrix[2][1] = -1.0*matrix[1][2];
    matrix[2][2] = matrix[1][1];
    matrix[2][3] = 0.0;
    matrix[2][0] = 0.0;
    matrix[3][3] = 1.0;
    matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
    this->Transform.Concatenate(matrix);

    // calc the twist
    // compute: vn X ( vup X vn)
    // and:     vn X ( y-axis X vn)
    // then find the angle between the two projected vectors
    //
    y_axis[0] = y_axis[2] = 0.0; y_axis[1] = 1.0;

    // bump the view normal if it is parallel to the y-axis
    //
    if ((vn[0] == 0.0) && (vn[2] == 0.0))
      vn[2] = 0.01*vn[1];

    // first project the view_up onto the view_plane
    //
    vtkMath::Cross(vup, vn, v1);
    vtkMath::Cross(vn, v1, v1);
    
    // then project the y-axis onto the view plane
    //
    vtkMath::Cross(y_axis, vn, v2);
    vtkMath::Cross(vn, v2, v2);
    
    // then find the angle between the two projected vectors
    //
    dot = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    mag = sqrt((double)(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]));
    mag *= sqrt((double)(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]));
    
    // make sure we dont divide by 0 
    if (mag != 0.0) 
      {
      cosang = dot / mag;
      if (cosang < -1.0) cosang = -1.0;
      if (cosang > 1.0) cosang = 1.0;
      theta = acos(cosang);
      }
    else
      theta = 0.0;
    
    // now see if the angle is positive or negative
    //
    vtkMath::Cross(v1, v2, v1);
    dot = v1[0]*vn[0] + v1[1]*vn[1] + v1[2]*vn[2];
    
    twist = (theta);
    if (dot < 0.0)
      twist = -twist;
    
    // now rotate z (twist) 
    matrix[0][0] = cos(-twist);
    matrix[0][1] = sin(-twist);
    matrix[0][2] = matrix[0][3] = 0.0;
    matrix[1][0] = -1.0*matrix[0][1];
    matrix[1][1] = matrix[0][0];
    matrix[1][2] = matrix[1][3] = 0.0;
    matrix[2][1] = 0.0;
    matrix[2][2] = 1.0;
    matrix[2][3] = 0.0;
    matrix[2][0] = 0.0;
    matrix[3][3] = 1.0;
    matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
    this->Transform.Concatenate(matrix);

    // rotate y by 180 to get the positive zaxis instead of negative
    this->Transform.RotateY(180);
    }

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

void vtkFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

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

// Description:
// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.  
void vtkFollower::Render(vtkRenderer *ren)
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  
  /* render the property */
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Device->SetProperty (this->Property);
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }

  /* render the texture */
  if (this->Texture) this->Texture->Render(ren);
    
  // make sure the device has the same matrix
  this->GetMatrix(*matrix);
  this->Device->SetUserMatrix(matrix);
  
  this->Device->Render(ren,this->Mapper);

  delete matrix;
}
