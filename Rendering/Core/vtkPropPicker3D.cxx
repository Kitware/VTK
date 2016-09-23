/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPropPicker3D.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWorldPointPicker.h"

vtkStandardNewMacro(vtkPropPicker3D);

vtkPropPicker3D::vtkPropPicker3D()
{
  this->PickFromProps = NULL;
}

vtkPropPicker3D::~vtkPropPicker3D()
{
}

// set up for a pick
void vtkPropPicker3D::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int vtkPropPicker3D::Pick(double selectionX, double selectionY,
                        double selectionZ, vtkRenderer *renderer)
{
  if ( this->PickFromList )
  {
    return this->PickProp(selectionX, selectionY, selectionZ, renderer, this->PickList);
  }
  else
  {
    return this->PickProp(selectionX, selectionY, selectionZ, renderer);
  }
}


// Pick from the given collection
int vtkPropPicker3D::PickProp(
  double selectionX, double selectionY, double selectionZ,
  vtkRenderer *renderer, vtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp(selectionX, selectionY, selectionZ, renderer);
  this->PickFromProps = NULL;
  return ret;
}



// Perform pick operation with selection point provided.
int vtkPropPicker3D::PickProp(
  double selectionX, double selectionY, double selectionZ,
  vtkRenderer *renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->PickPosition[0] = selectionX;
  this->PickPosition[1] = selectionY;
  this->PickPosition[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);


  // for each prop, that is packable
  // find the prop whose bounds
  // contain the pick points and whole center is closest to the
  // selection point
  // TODO need to handle AssemblyPaths
  vtkPropCollection *props = renderer->GetViewProps();

  vtkAssemblyPath *result = NULL;
  vtkCollectionSimpleIterator pit;
  props->InitTraversal(pit);
  vtkProp *prop = NULL;
  while ( (prop = props->GetNextProp(pit)) )
  {
    if (prop->GetPickable())
    {
      double *bnds = prop->GetBounds();
      if (bnds)
      {
        if (selectionX >= bnds[0] && selectionX <= bnds[1] &&
            selectionY >= bnds[2] && selectionY <= bnds[3] &&
            selectionZ >= bnds[4] && selectionZ <= bnds[5])
        {
          prop->InitPathTraversal();
          result = prop->GetNextPath();
        }
      }
    }
  }

  if (result)
  {
    result->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent,NULL);
  }
  this->SetPath(result);

  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  // Call Pick on the Prop that was picked, and return 1 for success
  if ( result )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void vtkPropPicker3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PickFromProps)
  {
    os << indent << "PickFrom List: " << this->PickFromProps << endl;
  }
  else
  {
    os << indent << "PickFrom List: (none)" << endl;
  }
}
