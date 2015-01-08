/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedAreaPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkRenderedAreaPicker.h"
#include "vtkObjectFactory.h"
#include "vtkMapper.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkImageMapper3D.h"
#include "vtkAbstractMapper3D.h"
#include "vtkProp.h"
#include "vtkLODProp3D.h"
#include "vtkActor.h"
#include "vtkPropCollection.h"
#include "vtkProp3DCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkProperty.h"
#include "vtkCommand.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkRenderedAreaPicker);

//--------------------------------------------------------------------------
vtkRenderedAreaPicker::vtkRenderedAreaPicker()
{
}

//--------------------------------------------------------------------------
vtkRenderedAreaPicker::~vtkRenderedAreaPicker()
{
}

//--------------------------------------------------------------------------
// Does what this class is meant to do.
int vtkRenderedAreaPicker::AreaPick(double x0, double y0, double x1, double y1,
                                    vtkRenderer *renderer)
{
  int picked = 0;
  vtkProp *propCandidate;
  vtkAbstractMapper3D *mapper = NULL;
  int pickable;

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  this->SelectionPoint[0] = (x0+x1)*0.5;
  this->SelectionPoint[1] = (y0+y1)*0.5;
  this->SelectionPoint[2] = 0.0;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  this->DefineFrustum(x0, y0, x1, y1, renderer);

  // Ask the renderer do the hardware pick
  vtkPropCollection* pickList = NULL;
  if(this->PickFromList)
    {
    pickList = this->PickList;
    }

  this->SetPath(renderer->PickPropFrom(x0, y0, x1, y1,pickList));

  // Software pick resulted in a hit.
  if ( this->Path )
    {
    picked = 1;

    //invoke the pick event
    propCandidate = this->Path->GetLastNode()->GetViewProp();

    //find the mapper and dataset corresponding to the picked prop
    pickable = this->TypeDecipher(propCandidate, &mapper);
    if ( pickable )
      {
      if ( mapper )
        {
        this->Mapper = mapper;
        vtkMapper *map1;
        vtkAbstractVolumeMapper *vmap;
        vtkImageMapper3D *imap;
        if ( (map1=vtkMapper::SafeDownCast(mapper)) != NULL )
          {
          this->DataSet = map1->GetInput();
          this->Mapper = map1;
          }
        else if ( (vmap=vtkAbstractVolumeMapper::SafeDownCast(mapper)) != NULL )
          {
          this->DataSet = vmap->GetDataSetInput();
          this->Mapper = vmap;
          }
        else if ( (imap=vtkImageMapper3D::SafeDownCast(mapper)) != NULL )
          {
          this->DataSet = imap->GetDataSetInput();
          this->Mapper = imap;
          }
        else
          {
          this->DataSet = NULL;
          }
        }//mapper
      }//pickable

    //go through list of props the renderer got for us and put only
    //the prop3Ds into this->Prop3Ds
    vtkPropCollection *pProps = renderer->GetPickResultProps();
    pProps->InitTraversal();

    vtkProp *prop;
    vtkAssemblyPath *path;
    while ((prop = pProps->GetNextProp()))
      {
      for ( prop->InitPathTraversal(); (path=prop->GetNextPath()); )
        {
        propCandidate = path->GetLastNode()->GetViewProp();
        pickable = this->TypeDecipher(propCandidate, &mapper);
        if ( pickable && !this->Prop3Ds->IsItemPresent(prop) )
          {
          this->Prop3Ds->AddItem(static_cast<vtkProp3D *>(prop));
          }
        }
      }

    // Invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent,NULL);
    }

  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  return picked;
}

//----------------------------------------------------------------------------
void vtkRenderedAreaPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
