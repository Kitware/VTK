/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBackgroundColorMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBackgroundColorMonitor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include <cstring>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBackgroundColorMonitor)

//-----------------------------------------------------------------------------
vtkBackgroundColorMonitor::vtkBackgroundColorMonitor()
        :
    UpTime(0),
    Gradient(false)
{
  memset(this->Color1,0,3*sizeof(double));
  memset(this->Color2,0,3*sizeof(double));
}

//-----------------------------------------------------------------------------
bool vtkBackgroundColorMonitor::StateChanged(vtkRenderer *ren)
{
  unsigned int oldUpTime = this->UpTime;
  this->Update(ren);
  if (oldUpTime != this->UpTime)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkBackgroundColorMonitor::Update(vtkRenderer *ren)
{
  // update colors
  double *color1 = ren->GetBackground();
  double *color2 = ren->GetBackground2();
  bool changed = false;
  for (int i=0; i<3; ++i)
  {
    if ( (this->Color1[i] != color1[i])
      || (this->Color2[i] != color2[i]) )
    {
      changed=true;
    }
    this->Color1[i] = color1[i];
    this->Color2[i] = color2[i];
  }
  // update gradient flag
  bool grad = ren->GetGradientBackground();
  if ( this->Gradient != grad )
  {
    changed = true;
  }
  this->Gradient = grad;
  // update mtime
  if (changed)
  {
    this->UpTime += 1;
  }
}

//-----------------------------------------------------------------------------
void vtkBackgroundColorMonitor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Gradient=" << this->Gradient << endl;
  os << indent << "Color1=";
  for (int q=0; q<3; ++q)
  {
     os << this->Color1[q] << " ";
  }
  os << endl;
  os << indent << "Color2=";
  for (int q=0; q<3; ++q)
  {
     os << this->Color2[q] << " ";
  }
  os << endl;
  os << indent << "UpTime=" << this->UpTime << endl;
}
