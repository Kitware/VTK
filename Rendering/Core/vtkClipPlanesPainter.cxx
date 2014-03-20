/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPlanesPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClipPlanesPainter.h"

#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkClipPlanesPainter)

vtkCxxSetObjectMacro(vtkClipPlanesPainter, ClippingPlanes, vtkPlaneCollection);

vtkInformationKeyMacro(vtkClipPlanesPainter, CLIPPING_PLANES, ObjectBase);
//-----------------------------------------------------------------------------
vtkClipPlanesPainter::vtkClipPlanesPainter()
{
  this->ClippingPlanes = 0;
}
//-----------------------------------------------------------------------------
vtkClipPlanesPainter::~vtkClipPlanesPainter()
{
  this->SetClippingPlanes(0);
}

//-----------------------------------------------------------------------------
void vtkClipPlanesPainter::ProcessInformation(vtkInformation* info)
{
  if (info->Has(CLIPPING_PLANES()))
    {
    this->SetClippingPlanes(vtkPlaneCollection::SafeDownCast(
        info->Get(CLIPPING_PLANES())));
    }

  this->Superclass::ProcessInformation(info);
}

//-----------------------------------------------------------------------------
void vtkClipPlanesPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
    os << indent << "ClippingPlanes:";
  if ( this->ClippingPlanes )
    {
    os << endl;
    this->ClippingPlanes->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << " (none)" << endl;
    }
}
