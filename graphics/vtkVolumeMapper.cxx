/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.cxx
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
#include "vtkVolumeMapper.h"

// Construct a vtkVolumeMapper with empty scalar input and clipping off.
vtkVolumeMapper::vtkVolumeMapper()
{
  this->ScalarInput = NULL;
  this->Clipping = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

vtkVolumeMapper::~vtkVolumeMapper()
{
  this->SetScalarInput((vtkStructuredPoints *)NULL);
}

void vtkVolumeMapper::Update()
{
  if ( this->ScalarInput )
    {
    this->ScalarInput->Update();
    }
}

// Get the bounds of the ScalarInput
float *vtkVolumeMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->ScalarInput ) 
    return bounds;
  else
    {
    this->ScalarInput->Update();
    this->ScalarInput->GetBounds(this->Bounds);
    return this->Bounds;
    }
}

// Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkVolumeMapper::GetBounds(float bounds[6])
{
  this->GetBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}

float *vtkVolumeMapper::GetCenter()
{
  this->GetBounds();
  for (int i=0; i<3; i++) 
    {
    this->Center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
    }
  return this->Center;
}

float vtkVolumeMapper::GetLength()
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

// Print the vtkVolumeMapper
void vtkVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->ScalarInput )
    {
    os << indent << "ScalarInput: (" << this->ScalarInput << ")\n";
    }
  else
    {
    os << indent << "ScalarInput: (none)\n";
    }

  os << indent << "Clipping: " << (this->Clipping ? "On\n" : "Off\n");
  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}

