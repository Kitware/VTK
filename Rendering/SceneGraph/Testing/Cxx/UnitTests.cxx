/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Mace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkActorNode.h"
#include "vtkCamera.h"
#include "vtkCameraNode.h"
#include "vtkLight.h"
#include "vtkLightNode.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRendererNode.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkViewNodeCollection.h"
#include "vtkViewNodeFactory.h"
#include "vtkWindowNode.h"

#include <string>
namespace {
  std::string resultS = "";
}

//-----------------------------------------------------------------------
//ViewNode subclasses specialized for this test
class vtkMyActorNode : public vtkActorNode
{
public:
  static vtkMyActorNode* New();
  vtkTypeMacro(vtkMyActorNode, vtkActorNode);
  virtual void Render(bool prepass) {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyActorNode() {};
  ~vtkMyActorNode() {};
};
vtkStandardNewMacro(vtkMyActorNode);

class vtkMyCameraNode : public vtkCameraNode
{
public:
  static vtkMyCameraNode* New();
  vtkTypeMacro(vtkMyCameraNode, vtkCameraNode);
  virtual void Render(bool prepass) {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyCameraNode() {};
  ~vtkMyCameraNode() {};
};
vtkStandardNewMacro(vtkMyCameraNode);

class vtkMyLightNode : public vtkLightNode
{
public:
  static vtkMyLightNode* New();
  vtkTypeMacro(vtkMyLightNode, vtkLightNode);
  virtual void Render( bool prepass) {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyLightNode() {};
  ~vtkMyLightNode() {};
};
vtkStandardNewMacro(vtkMyLightNode);

class vtkMyRendererNode : public vtkRendererNode
{
public:
  static vtkMyRendererNode* New();
  vtkTypeMacro(vtkMyRendererNode, vtkRendererNode);
  virtual void Render(bool prepass) {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyRendererNode() {};
  ~vtkMyRendererNode() {};
};
vtkStandardNewMacro(vtkMyRendererNode);

class vtkMyWindowNode : public vtkWindowNode
{
public:
  static vtkMyWindowNode* New();
  vtkTypeMacro(vtkMyWindowNode, vtkWindowNode);
  virtual void Render(bool prepass) {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  vtkMyWindowNode() {};
  ~vtkMyWindowNode() {};
};
vtkStandardNewMacro(vtkMyWindowNode);

//------------------------------------------------------------------------------

//builders that produce the specialized ViewNodes
vtkViewNode *act_maker()
{
  vtkMyActorNode *vn = vtkMyActorNode::New();
  cerr << "make actor node " << vn << endl;
  resultS += "make actor\n";
  return vn;
}

vtkViewNode *cam_maker()
{
  vtkMyCameraNode *vn = vtkMyCameraNode::New();
  cerr << "make camera node " << vn << endl;
  resultS += "make camera\n";
  return vn;
}

vtkViewNode *light_maker()
{
  vtkMyLightNode *vn = vtkMyLightNode::New();
  cerr << "make light node " << vn << endl;
  resultS += "make light\n";
  return vn;
}

vtkViewNode *ren_maker()
{
  vtkMyRendererNode *vn = vtkMyRendererNode::New();
  cerr << "make renderer node " << vn << endl;
  resultS += "make renderer\n";
  return vn;
}

vtkViewNode *win_maker()
{
  vtkMyWindowNode *vn = vtkMyWindowNode::New();
  cerr << "make window node " << vn << endl;
  resultS += "make window\n";
  return vn;
}

//exercises the scene graph related classes
int UnitTests( int vtkNotUsed(argc), char *vtkNotUsed(argv)[] )
{
  vtkWindowNode *wvn = vtkWindowNode::New();
  cerr << "made " << wvn << endl;

  vtkViewNodeCollection *vnc = vtkViewNodeCollection::New();
  cerr << "made " << vnc << endl;
  vnc->AddItem(wvn);
  vnc->PrintSelf(cerr, vtkIndent(0));
  wvn->Delete();
  vnc->Delete();

  vtkViewNode *vn = NULL;
  vtkViewNodeFactory *vnf = vtkViewNodeFactory::New();
  cerr << "CREATE pre override" << endl;
  vnc = NULL;
  vn = vnf->CreateNode(vnc);
  if (vn)
  {
    cerr << "Shouldn't have made anything" << endl;
    return 1;
  }
  cerr << "factory made nothing as it should have" << endl;

  vtkRenderWindow *rwin = vtkRenderWindow::New();
  vnf->RegisterOverride(rwin->GetClassName(), win_maker);
  cerr << "CREATE node for renderwindow" << endl;
  vn = vnf->CreateNode(rwin);

  cerr << "factory makes" << endl;
  cerr << vn << endl;
  cerr << "BUILD [" << endl;
  vn->Traverse(vtkViewNode::build);
  cerr << "]" << endl;

  cerr << "add renderer" << endl;
  vtkRenderer *ren = vtkRenderer::New();
  vnf->RegisterOverride(ren->GetClassName(), ren_maker);
  rwin->AddRenderer(ren);

  vtkLight *light = vtkLight::New();
  vnf->RegisterOverride(light->GetClassName(), light_maker);
  ren->AddLight(light);
  light->Delete();

  vtkCamera *cam = vtkCamera::New();
  vnf->RegisterOverride(cam->GetClassName(), cam_maker);
  cam->Delete();

  vtkActor *actor = vtkActor::New();
  vnf->RegisterOverride(actor->GetClassName(), act_maker);
  ren->AddActor(actor);
  actor->Delete();

  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkPolyDataMapper *pmap = vtkPolyDataMapper::New();
  pmap->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(pmap);
  rwin->Render();
  sphere->Delete();
  pmap->Delete();

  cerr << "BUILD [" << endl;
  vn->Traverse(vtkViewNode::build);
  cerr << "]" << endl;
  cerr << "SYNCHRONIZE [" << endl;
  vn->Traverse(vtkViewNode::synchronize);
  cerr << "]" << endl;
  cerr << "RENDER [" << endl;
  vn->Traverse(vtkViewNode::render);
  cerr << "]" << endl;

  vn->Delete();
  ren->Delete();
  rwin->Delete();

  vnf->Delete();

  cerr << "Results is [" << endl;
  cerr << resultS << "]" << endl;
  std::string ok_res = "make window\nmake renderer\nmake light\nmake actor\nmake camera\nRender vtkMyWindowNode\nRender vtkMyRendererNode\nRender vtkMyLightNode\nRender vtkMyActorNode\nRender vtkMyCameraNode\n";
  if (resultS != ok_res)
  {
    cerr << "Which does not match [" << endl;
    cerr << ok_res << "]" << endl;
    return 1;
  }
  return 0;
}
