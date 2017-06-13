/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOptiXViewNodeFactory.h"
#include "vtkObjectFactory.h"

#include "vtkOptiXActorNode.h"
#include "vtkOptiXCameraNode.h"
#include "vtkOptiXCompositePolyDataMapper2Node.h"
#include "vtkOptiXLightNode.h"
#include "vtkOptiXRendererNode.h"
#include "vtkOptiXPolyDataMapperNode.h"

vtkViewNode *ren_maker()
{
  vtkOptiXRendererNode *vn = vtkOptiXRendererNode::New();
  return vn;
}

vtkViewNode *act_maker()
{
  vtkOptiXActorNode *vn = vtkOptiXActorNode::New();
  return vn;
}

vtkViewNode *cam_maker()
{
  vtkOptiXCameraNode *vn = vtkOptiXCameraNode::New();
  return vn;
}

vtkViewNode *light_maker()
{
  vtkOptiXLightNode *vn = vtkOptiXLightNode::New();
  return vn;
}

vtkViewNode *pd_maker()
{
  vtkOptiXPolyDataMapperNode *vn = vtkOptiXPolyDataMapperNode::New();
  return vn;
}

vtkViewNode *cpd_maker()
{
  vtkOptiXCompositePolyDataMapper2Node *vn =
    vtkOptiXCompositePolyDataMapper2Node::New();
  return vn;
}

//============================================================================
vtkStandardNewMacro(vtkOptiXViewNodeFactory);

//------------------------------------------------------------------------------
vtkOptiXViewNodeFactory::vtkOptiXViewNodeFactory()
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

//------------------------------------------------------------------------------
vtkOptiXViewNodeFactory::~vtkOptiXViewNodeFactory()
{
}

//------------------------------------------------------------------------------
void vtkOptiXViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
