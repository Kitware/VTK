/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayViewNodeFactory.h"
#include "vtkObjectFactory.h"

#include "vtkOsprayActorNode.h"
#include "vtkOsprayCameraNode.h"
#include "vtkOsprayCompositePolyDataMapper2Node.h"
#include "vtkOsprayLightNode.h"
#include "vtkOsprayRendererNode.h"
#include "vtkOsprayPolyDataMapperNode.h"

vtkViewNode *ren_maker()
{
  vtkOsprayRendererNode *vn = vtkOsprayRendererNode::New();
  return vn;
}

vtkViewNode *act_maker()
{
  vtkOsprayActorNode *vn = vtkOsprayActorNode::New();
  return vn;
}

vtkViewNode *cam_maker()
{
  vtkOsprayCameraNode *vn = vtkOsprayCameraNode::New();
  return vn;
}

vtkViewNode *light_maker()
{
  vtkOsprayLightNode *vn = vtkOsprayLightNode::New();
  return vn;
}

vtkViewNode *pd_maker()
{
  vtkOsprayPolyDataMapperNode *vn = vtkOsprayPolyDataMapperNode::New();
  return vn;
}

vtkViewNode *cpd_maker()
{
  vtkOsprayCompositePolyDataMapper2Node *vn = vtkOsprayCompositePolyDataMapper2Node::New();
  return vn;
}

//============================================================================
vtkStandardNewMacro(vtkOsprayViewNodeFactory);

//----------------------------------------------------------------------------
vtkOsprayViewNodeFactory::vtkOsprayViewNodeFactory()
{
  //see vtkRenderWindow::GetRenderLibrary
  this->RegisterOverride("vtkOpenGLRenderer", ren_maker);
  this->RegisterOverride("vtkOpenGLActor", act_maker);
  this->RegisterOverride("vtkPVLODActor", act_maker);
  this->RegisterOverride("vtkOpenGLCamera", cam_maker);
  this->RegisterOverride("vtkOpenGLLight", light_maker);
  this->RegisterOverride("vtkPainterPolyDataMapper", pd_maker);
  this->RegisterOverride("vtkOpenGLPolyDataMapper", pd_maker);
  this->RegisterOverride("vtkCompositePolyDataMapper2", cpd_maker);
}

//----------------------------------------------------------------------------
vtkOsprayViewNodeFactory::~vtkOsprayViewNodeFactory()
{
}

//----------------------------------------------------------------------------
void vtkOsprayViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
