// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariViewNodeFactory.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariCameraNode.h"
#include "vtkAnariCompositePolyDataMapperNode.h"
#include "vtkAnariFollowerNode.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariPolyDataMapperNode.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariVolumeMapperNode.h"
#include "vtkAnariVolumeNode.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkViewNode* ren_maker()
{
  vtkAnariRendererNode* vn = vtkAnariRendererNode::New();
  return vn;
}

vtkViewNode* act_maker()
{
  vtkAnariActorNode* vn = vtkAnariActorNode::New();
  return vn;
}

vtkViewNode* vol_maker()
{
  vtkAnariVolumeNode* vn = vtkAnariVolumeNode::New();
  return vn;
}

vtkViewNode* cam_maker()
{
  vtkAnariCameraNode* vn = vtkAnariCameraNode::New();
  return vn;
}

vtkViewNode* light_maker()
{
  vtkAnariLightNode* vn = vtkAnariLightNode::New();
  return vn;
}

vtkViewNode* pd_maker()
{
  vtkAnariPolyDataMapperNode* vn = vtkAnariPolyDataMapperNode::New();
  return vn;
}

vtkViewNode* vm_maker()
{
  vtkAnariVolumeMapperNode* vn = vtkAnariVolumeMapperNode::New();
  return vn;
}

vtkViewNode* cpd_maker()
{
  vtkAnariCompositePolyDataMapperNode* vn = vtkAnariCompositePolyDataMapperNode::New();
  return vn;
}

vtkViewNode* fol_maker()
{
  vtkAnariFollowerNode* vn = vtkAnariFollowerNode::New();
  return vn;
}

//============================================================================
vtkStandardNewMacro(vtkAnariViewNodeFactory);

//----------------------------------------------------------------------------
vtkAnariViewNodeFactory::vtkAnariViewNodeFactory()
{
  this->RegisterOverride("vtkOpenGLRenderer", ren_maker);
  this->RegisterOverride("vtkOpenGLActor", act_maker);
  this->RegisterOverride("vtkPVLODActor", act_maker);
  this->RegisterOverride("vtkOpenGLCamera", cam_maker);
  this->RegisterOverride("vtkFollower", fol_maker);
  this->RegisterOverride("vtkOpenGLLight", light_maker);
  this->RegisterOverride("vtkPVLight", light_maker);
  this->RegisterOverride("vtkPainterPolyDataMapper", pd_maker);
  this->RegisterOverride("vtkOpenGLPolyDataMapper", pd_maker);
  // VTK_DEPRECATED_IN_9_3_0: Remove CPDM2 override after vtkCompositePolyDataMapper2 is removed
  this->RegisterOverride("vtkCompositePolyDataMapper2", cpd_maker);
  this->RegisterOverride("vtkCompositePolyDataMapper", cpd_maker);
  this->RegisterOverride("vtkVolume", vol_maker);
  this->RegisterOverride("vtkPVLODVolume", vol_maker);
  this->RegisterOverride("vtkSmartVolumeMapper", vm_maker);
  this->RegisterOverride("vtkAnariVolumeMapper", vm_maker);
  this->RegisterOverride("vtkMultiBlockVolumeMapper", vm_maker);
  this->RegisterOverride("vtkOpenGLGPUVolumeRayCastMapper", vm_maker);
}

//----------------------------------------------------------------------------
void vtkAnariViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
