/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.cxx
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
#include "vtkLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"

// Description:
// Creates a vtkLODActor with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). NumberOfCloudPoints is set to 150.
vtkLODActor::vtkLODActor()
{
  this->NumberOfCloudPoints = 150;
  this->Timings[0] = -2; // highest LOD
  this->Timings[1] = -2;
  this->Timings[2] = -2; // lowest LOD
  // get a hardware dependent actor and mappers
  this->Device = vtkActor::New();
  this->MediumMapper = vtkPolyDataMapper::New();
  this->LowMapper = vtkPolyDataMapper::New();
}

vtkLODActor::~vtkLODActor()
{
  this->Device->Delete();
  this->MediumMapper->Delete();
  this->LowMapper->Delete();
}


// Description:
// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.  
void vtkLODActor::Render(vtkRenderer *ren)
{
  int choice;
  float myTime;
  double aTime;
  static int refreshCount = 0; // every 97 calls decay some timings
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  
  // figure out how much time we have to render
  myTime = ren->GetAllocatedRenderTime();
  myTime /= (ren->GetActors())->GetNumberOfItems();
  
  if (this->GetMTime() > this->BuildTime || 
      this->Mapper->GetMTime() > this->BuildTime)
    {
    // make sure the filters are connected
    this->PointSource.SetRadius(0);
    this->PointSource.SetNumberOfPoints(1);
    this->MediumMapper->SetInput(this->Glyph3D.GetOutput());
    this->MediumMapper->SetScalarRange(this->Mapper->GetScalarRange());
    this->MediumMapper->SetScalarVisibility(this->Mapper->GetScalarVisibility());
    this->MaskPoints.SetInput(this->Mapper->GetInput());
    this->MaskPoints.SetMaximumNumberOfPoints(this->NumberOfCloudPoints);
    this->MaskPoints.SetRandomMode(1);
    this->Glyph3D.SetInput(this->MaskPoints.GetOutput());
    this->Glyph3D.SetSource(this->PointSource.GetOutput());
    this->LowMapper->SetInput(this->OutlineFilter.GetOutput());
    this->OutlineFilter.SetInput(this->Mapper->GetInput());
    
    this->Timings[0] = -2;
    this->Timings[1] = -2;
    this->Timings[2] = -2;
    this->BuildTime.Modified();
    }

  // figure out which resolution to use
  if ((myTime > this->Timings[0])|| (myTime == 0))
    {
    choice = 0;
    }
  else if (myTime > this->Timings[1])
    {
    choice = 1;
    }
  else
    {
    choice = 2;
    }
  aTime = vtkTimerLog::GetCurrentTime();
  
  /* render the property */
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }
  this->Device->SetProperty(this->Property);
  

  /* render the texture */
  if (this->Texture) this->Texture->Render(ren);
    
  // make sure the device has the same matrix
  this->GetMatrix(*matrix);
  this->Device->SetUserMatrix(matrix);
  
  if (!choice)
    {
    this->Device->Render(ren,this->Mapper);
    }
  if (choice == 1)
    {
    this->Device->Render(ren,this->MediumMapper);
    }
  if (choice == 2)
    {
    this->Device->Render(ren,this->LowMapper);
    }
  
  if (this->Timings[choice] == -2)
    {
    this->Timings[choice] = -1;
    }
  else
    {
    if (!(refreshCount % 97))
      {
      if (this->Timings[0] < (myTime*2.0))
	{
	this->Timings[0] = -1;
	}
      this->Timings[1] = -1;
      this->Timings[2] = -1;
      }
    
    this->Timings[choice] = (float)(vtkTimerLog::GetCurrentTime() - aTime);
    }

  refreshCount++;
  delete matrix;
}

void vtkLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);
  os << indent << "Timings: (" << this->Timings[0] << ", " 
     << this->Timings[1] << ", " << this->Timings[2] << ")\n";
}

