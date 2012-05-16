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
#include "vtkBoundingBox.h"

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
//-----------------------------------------------------------------------------
void vtkClipPlanesPainter::UpdateBounds(double bounds[6])
{
  if(!vtkBoundingBox::IsValid(bounds))
    {
    return;
    }
  vtkPlaneCollection* planes =this->ClippingPlanes;
  if(planes)
    {
    int numPlanes = planes->GetNumberOfItems();
    for(int i=0; i<numPlanes; i++)
      {
      vtkPlane *plane = planes->GetItem(i);
      if(plane)
        {
        double n[3],p[3];
        plane->GetNormal(n);
        plane->GetOrigin(p);
        vtkBoundingBox bb(bounds);
        if(bb.IntersectPlane(p,n))
          {
          bb.GetBounds(bounds);
          }
        }
      }
    }

}
