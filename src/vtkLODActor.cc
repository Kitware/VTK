/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <time.h>
#include <sys/time.h>
#include "vtkLODActor.hh"
#include "vtkMath.hh"

// Description:
// Creates an actor with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). 
vtkLODActor::vtkLODActor()
{
  this->LowThreshold = 1.4;
  this->MediumThreshold = 1.2;
  this->Timings[0] = -2; // highest LOD
  this->Timings[1] = -2;
  this->Timings[2] = -2; // lowest LOD
}

vtkLODActor::~vtkLODActor()
{
}

// Description:
// This causes the actor to be rendered. It in turn will render the actor's
// property and then mapper.  
void vtkLODActor::Render(vtkRenderer *ren)
{
  static vtkMath math;
  int choice;
  struct timeval time1,time2;
  struct timezone zone;
  float myTime;
  
  // figure out how much time we have to rtender
  myTime = ren->GetAllocatedRenderTime();
  myTime /= (ren->GetActors())->GetNumberOfItems();
  
  if (this->GetMTime() > this->BuildTime || 
      this->Mapper->GetMTime() > this->BuildTime)
    {
    // make sure the filters are connected
    this->PointSource.SetRadius(0);
    this->PointSource.SetNumberOfPoints(1);
    this->MediumMapper.SetInput(&this->Glyph3D);
    this->MediumMapper.SetScalarRange(this->Mapper->GetScalarRange());
    this->MediumMapper.SetScalarsVisible(this->Mapper->GetScalarsVisible());
    this->MaskPoints.SetInput(this->Mapper->GetInput());
    this->MaskPoints.SetMaximumNumberOfPoints(120);
    this->MaskPoints.SetRandomMode(1);
    this->Glyph3D.SetInput(&this->MaskPoints);
    this->Glyph3D.SetSource(&this->PointSource);
    this->LowMapper.SetInput(&this->OutlineFilter);
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
  gettimeofday(&time1,&zone);
  
  /* render the property */
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(ren);

  /* render the texture */
  if (this->Texture) this->Texture->Render(ren);
    
  if (!choice)
    {
    this->Mapper->Render(ren);
    }
  if (choice == 1)
    {
    this->MediumMapper.Render(ren);
    }
  if (choice == 2)
    {
    this->LowMapper.Render(ren);
    }
  
  if (this->Timings[choice] == -2)
    {
    this->Timings[choice] = -1;
    }
  else
    {
    gettimeofday(&time2,&zone);
    this->Timings[choice] = time2.tv_sec - time1.tv_sec;
    this->Timings[choice] += (time2.tv_usec - time1.tv_usec)/1000000.0;
    }
}

void vtkLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);
  os << indent << "Timings: (" << this->Timings[0] << ", " 
     << this->Timings[1] << ", " << this->Timings[2] << ")\n";
}

