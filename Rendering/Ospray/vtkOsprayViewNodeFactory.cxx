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
#include "vtkOsprayLightNode.h"
#include "vtkOsprayRendererNode.h"
#include "vtkOsprayWindowNode.h"

vtkViewNode *win_maker()
{
  vtkOsprayWindowNode *vn = vtkOsprayWindowNode::New();
  return vn;
}

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

//============================================================================
vtkStandardNewMacro(vtkOsprayViewNodeFactory);

//----------------------------------------------------------------------------
vtkOsprayViewNodeFactory::vtkOsprayViewNodeFactory()
{
  //TODO: better to introspect somehow and always get a definitive list.
  //Until then just grep override in Rendering/OpenGL*/CMakeLists.txt
  //to come up with something like this.
  this->RegisterOverride("vtkCocoaRenderWindow", win_maker);
  this->RegisterOverride("vtkXOpenGLRenderWindow", win_maker);
  this->RegisterOverride("vtkOSOpenGLRenderWindow", win_maker);
  this->RegisterOverride("vtkWin32OpenGLRenderWindow", win_maker);
  this->RegisterOverride("vtkCocoaRenderWindow", win_maker);
  this->RegisterOverride("vtkCarbonRenderWindow", win_maker);
  this->RegisterOverride("vtkEGLRenderWindow", win_maker);
  this->RegisterOverride("vtkIOSRenderWindow", win_maker);

  //see vtkRenderWindow::GetRenderLibrary
  this->RegisterOverride("vtkOpenGLRenderer", ren_maker);
  this->RegisterOverride("vtkOpenGLActor", act_maker);
  this->RegisterOverride("vtkPVLODActor", act_maker);
  this->RegisterOverride("vtkOpenGLCamera", cam_maker);
  this->RegisterOverride("vtkOpenGLLight", light_maker);
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
