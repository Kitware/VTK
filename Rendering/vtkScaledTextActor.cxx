/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.cxx
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
#include "vtkScaledTextActor.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkScaledTextActor, "1.26");
vtkStandardNewMacro(vtkScaledTextActor);

vtkScaledTextActor::vtkScaledTextActor()
{
  this->Position2Coordinate->SetValue(0.6, 0.1);
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.2,0.85);
  this->SetScaledText(1);
}

