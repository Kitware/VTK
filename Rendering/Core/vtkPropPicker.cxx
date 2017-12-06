/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPropPicker.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWorldPointPicker.h"

vtkStandardNewMacro(vtkPropPicker);

vtkPropPicker::vtkPropPicker()
{
  this->PickFromProps = nullptr;
  this->WorldPointPicker = vtkWorldPointPicker::New();
}

vtkPropPicker::~vtkPropPicker()
{
  this->WorldPointPicker->Delete();
}

// set up for a pick
void vtkPropPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int vtkPropPicker::Pick(double selectionX, double selectionY,
                        double vtkNotUsed(z), vtkRenderer *renderer)
{
  if ( this->PickFromList )
  {
    return this->PickProp(selectionX, selectionY, renderer, this->PickList);
  }
  else
  {
    return this->PickProp(selectionX, selectionY, renderer);
  }
}


// Pick from the given collection
int vtkPropPicker::PickProp(double selectionX, double selectionY,
                            vtkRenderer *renderer, vtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp(selectionX, selectionY, renderer);
  this->PickFromProps = nullptr;
  return ret;
}



// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkPropPicker::PickProp(double selectionX, double selectionY,
                            vtkRenderer *renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = 0;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,nullptr);

  // Have the renderer do the hardware pick
  this->SetPath(
    renderer->PickPropFrom(selectionX, selectionY, this->PickFromProps));

  // If there was a pick then find the world x,y,z for the pick, and invoke
  // its pick method.
  if ( this->Path )
  {
    this->WorldPointPicker->Pick(selectionX, selectionY, 0, renderer);
    this->WorldPointPicker->GetPickPosition(this->PickPosition);
    this->Path->GetLastNode()->GetViewProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent,nullptr);
  }

  this->InvokeEvent(vtkCommand::EndPickEvent,nullptr);

  // Call Pick on the Prop that was picked, and return 1 for success
  if ( this->Path )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Pick from the given collection
int vtkPropPicker::Pick3DPoint(double pos[3], vtkRenderer *renderer)
{
  if ( this->PickFromList )
  {
    return this->PickProp3DPoint(pos, renderer, this->PickList);
  }
  else
  {
    return this->PickProp3DPoint(pos, renderer);
  }
}

// Pick from the given collection
int vtkPropPicker::PickProp3DPoint(
  double pos[3],
  vtkRenderer *renderer, vtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp3DPoint(pos, renderer);
  this->PickFromProps = nullptr;
  return ret;
}



// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkPropPicker::PickProp3DPoint(double pos[3], vtkRenderer *renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = pos[0];
  this->SelectionPoint[1] = pos[1];
  this->SelectionPoint[2] = pos[2];

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  // for each prop, that is packable
  // find the prop whose bounds
  // contain the pick points and whole center is closest to the
  // selection point
  // TODO need to handle AssemblyPaths
  vtkPropCollection *props = renderer->GetViewProps();

  vtkAssemblyPath *result = nullptr;
  vtkCollectionSimpleIterator pit;
  props->InitTraversal(pit);
  vtkProp *prop = nullptr;
  while ( (prop = props->GetNextProp(pit)) )
  {
    if (prop->GetPickable())
    {
      const double *bnds = prop->GetBounds();
      if (bnds)
      {
        if (pos[0] >= bnds[0] && pos[0] <= bnds[1] &&
            pos[1] >= bnds[2] && pos[1] <= bnds[3] &&
            pos[2] >= bnds[4] && pos[2] <= bnds[5])
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
    this->InvokeEvent(vtkCommand::PickEvent,nullptr);
  }
  this->SetPath(result);

  this->InvokeEvent(vtkCommand::EndPickEvent,nullptr);

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


void vtkPropPicker::PrintSelf(ostream& os, vtkIndent indent)
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
