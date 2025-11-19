// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkActorNode.h"
#include "vtkCamera.h"
#include "vtkCameraNode.h"
#include "vtkLight.h"
#include "vtkLightNode.h"
#include "vtkMapperNode.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererNode.h"
#include "vtkSphereSource.h"
#include "vtkViewNodeFactory.h"
#include "vtkWindowNode.h"

#include <iostream>
#include <string>

namespace
{
std::string resultS;
}

//------------------------------------------------------------------------------
// ViewNode subclasses specialized for this test
class vtkMyActorNode : public vtkActorNode
{
public:
  static vtkMyActorNode* New();
  vtkTypeMacro(vtkMyActorNode, vtkActorNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyActorNode() = default;
  ~vtkMyActorNode() override = default;
};
vtkStandardNewMacro(vtkMyActorNode);

class vtkMyCameraNode : public vtkCameraNode
{
public:
  static vtkMyCameraNode* New();
  vtkTypeMacro(vtkMyCameraNode, vtkCameraNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyCameraNode() = default;
  ~vtkMyCameraNode() override = default;
};
vtkStandardNewMacro(vtkMyCameraNode);

class vtkMyLightNode : public vtkLightNode
{
public:
  static vtkMyLightNode* New();
  vtkTypeMacro(vtkMyLightNode, vtkLightNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyLightNode() = default;
  ~vtkMyLightNode() override = default;
};
vtkStandardNewMacro(vtkMyLightNode);

class vtkMyMapperNode : public vtkMapperNode
{
public:
  static vtkMyMapperNode* New();
  vtkTypeMacro(vtkMyMapperNode, vtkMapperNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyMapperNode() = default;
  ~vtkMyMapperNode() override = default;
};
vtkStandardNewMacro(vtkMyMapperNode);

class vtkMyRendererNode : public vtkRendererNode
{
public:
  static vtkMyRendererNode* New();
  vtkTypeMacro(vtkMyRendererNode, vtkRendererNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyRendererNode() = default;
  ~vtkMyRendererNode() override = default;
};
vtkStandardNewMacro(vtkMyRendererNode);

class vtkMyWindowNode : public vtkWindowNode
{
public:
  static vtkMyWindowNode* New();
  vtkTypeMacro(vtkMyWindowNode, vtkWindowNode);
  void Render(bool prepass) override
  {
    if (prepass)
    {
      std::cerr << "Render " << this << " " << this->GetClassName() << std::endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyWindowNode() = default;
  ~vtkMyWindowNode() override = default;
};
vtkStandardNewMacro(vtkMyWindowNode);

//------------------------------------------------------------------------------

// builders that produce the specialized ViewNodes
vtkViewNode* act_maker()
{
  vtkMyActorNode* vn = vtkMyActorNode::New();
  std::cerr << "make actor node " << vn << std::endl;
  resultS += "make actor\n";
  return vn;
}

vtkViewNode* cam_maker()
{
  vtkMyCameraNode* vn = vtkMyCameraNode::New();
  std::cerr << "make camera node " << vn << std::endl;
  resultS += "make camera\n";
  return vn;
}

vtkViewNode* light_maker()
{
  vtkMyLightNode* vn = vtkMyLightNode::New();
  std::cerr << "make light node " << vn << std::endl;
  resultS += "make light\n";
  return vn;
}

vtkViewNode* mapper_maker()
{
  vtkMyMapperNode* vn = vtkMyMapperNode::New();
  std::cerr << "make mapper node " << vn << std::endl;
  resultS += "make mapper\n";
  return vn;
}

vtkViewNode* ren_maker()
{
  vtkMyRendererNode* vn = vtkMyRendererNode::New();
  std::cerr << "make renderer node " << vn << std::endl;
  resultS += "make renderer\n";
  return vn;
}

vtkViewNode* win_maker()
{
  vtkMyWindowNode* vn = vtkMyWindowNode::New();
  std::cerr << "make window node " << vn << std::endl;
  resultS += "make window\n";
  return vn;
}

// exercises the scene graph related classes
int UnitTests(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkWindowNode* wvn = vtkWindowNode::New();
  std::cerr << "made " << wvn << std::endl;
  wvn->Delete();

  vtkViewNode* vn = nullptr;
  vtkViewNodeFactory* vnf = vtkViewNodeFactory::New();
  std::cerr << "CREATE pre override" << std::endl;
  vn = vnf->CreateNode(nullptr);
  if (vn)
  {
    std::cerr << "Shouldn't have made anything" << std::endl;
    return 1;
  }
  std::cerr << "factory made nothing as it should have" << std::endl;

  vtkRenderWindow* rwin = vtkRenderWindow::New();
  vnf->RegisterOverride(rwin->GetClassName(), win_maker);
  std::cerr << "CREATE node for renderwindow" << std::endl;
  vn = vnf->CreateNode(rwin);

  std::cerr << "factory makes" << std::endl;
  std::cerr << vn << std::endl;
  std::cerr << "BUILD [" << std::endl;
  vn->Traverse(vtkViewNode::build);
  std::cerr << "]" << std::endl;

  std::cerr << "add renderer" << std::endl;
  vtkRenderer* ren = vtkRenderer::New();
  vnf->RegisterOverride(ren->GetClassName(), ren_maker);
  rwin->AddRenderer(ren);

  vtkLight* light = vtkLight::New();
  vnf->RegisterOverride(light->GetClassName(), light_maker);
  ren->AddLight(light);
  light->Delete();

  vnf->RegisterOverride("vtkMapper", mapper_maker);

  vtkCamera* cam = vtkCamera::New();
  vnf->RegisterOverride(cam->GetClassName(), cam_maker);
  cam->Delete();

  vtkActor* actor = vtkActor::New();
  vnf->RegisterOverride(actor->GetClassName(), act_maker);
  ren->AddActor(actor);
  actor->Delete();

  vtkSphereSource* sphere = vtkSphereSource::New();
  vtkPolyDataMapper* pmap = vtkPolyDataMapper::New();
  pmap->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(pmap);
  rwin->Render();
  sphere->Delete();
  pmap->Delete();

  std::cerr << "BUILD [" << std::endl;
  vn->Traverse(vtkViewNode::build);
  std::cerr << "]" << std::endl;
  std::cerr << "SYNCHRONIZE [" << std::endl;
  vn->Traverse(vtkViewNode::synchronize);
  std::cerr << "]" << std::endl;
  std::cerr << "RENDER [" << std::endl;
  vn->Traverse(vtkViewNode::render);
  std::cerr << "]" << std::endl;

  vn->Delete();
  ren->Delete();
  rwin->Delete();

  vnf->Delete();

  std::cerr << "Results is [" << std::endl;
  std::cerr << resultS << "]" << std::endl;
  std::string ok_res = "make window\nmake renderer\nmake light\nmake actor\nmake camera\nmake "
                       "mapper\nRender vtkMyWindowNode\nRender vtkMyRendererNode\nRender "
                       "vtkMyLightNode\nRender vtkMyActorNode\nRender vtkMyMapperNode\nRender "
                       "vtkMyCameraNode\n";
  if (resultS != ok_res)
  {
    std::cerr << "Which does not match [" << std::endl;
    std::cerr << ok_res << "]" << std::endl;
    return 1;
  }
  return 0;
}
