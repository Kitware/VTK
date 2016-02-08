/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayCameraNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayCameraNode.h"

#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"

//============================================================================
vtkStandardNewMacro(vtkOsprayCameraNode);

//----------------------------------------------------------------------------
vtkOsprayCameraNode::vtkOsprayCameraNode()
{
}

//----------------------------------------------------------------------------
vtkOsprayCameraNode::~vtkOsprayCameraNode()
{
}

//----------------------------------------------------------------------------
void vtkOsprayCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsprayCameraNode::ORender(int *TiledSize, void *camera)
{
  OSPCamera ospCamera = (OSPCamera)camera;
  ospSetf(ospCamera,"aspect",float(TiledSize[0])/float(TiledSize[1]));
  ospSetf(ospCamera,"fovy",this->ViewAngle);
  ospSet3f(ospCamera,"pos",this->Position[0], this->Position[1], this->Position[2]);
  ospSet3f(ospCamera,"up",this->ViewUp[0], this->ViewUp[1], this->ViewUp[2]);
  ospSet3f(ospCamera,"dir",
           this->FocalPoint[0]-this->Position[0],
           this->FocalPoint[1]-this->Position[1],
           this->FocalPoint[2]-this->Position[2]);
  ospCommit(ospCamera);
}
