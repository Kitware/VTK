/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImagePermute.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImagePermute);

vtkImagePermute::vtkImagePermute()
{
  this->FilteredAxes[0] = 0;
  this->FilteredAxes[1] = 1;
  this->FilteredAxes[2] = 2;
}

void vtkImagePermute::SetFilteredAxes(int newx, int newy, int newz)
{
  static double axes[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };

  this->SetResliceAxesDirectionCosines(axes[newx], axes[newy], axes[newz]);

  this->FilteredAxes[0] = newx;
  this->FilteredAxes[1] = newy;
  this->FilteredAxes[2] = newz;
}

void vtkImagePermute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FilteredAxes: ( "
     << this->FilteredAxes[0] << ", "
     << this->FilteredAxes[1] << ", "
     << this->FilteredAxes[2] << " )\n";
}

