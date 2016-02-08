/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayWindowNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayWindowNode.h"

#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkOsprayRendererNode.h"
#include "vtkRenderWindow.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"
#include <exception>

//============================================================================
vtkStandardNewMacro(vtkOsprayWindowNode);

//----------------------------------------------------------------------------
vtkOsprayWindowNode::vtkOsprayWindowNode()
{
  int ac = 2;
  const char* av[] = {"pvOSPRay\0","--osp:verbose\0"};
  try
    {
    ospInit(&ac, av);
    }
  catch (std::runtime_error &e)
    {
    //todo: request addition of ospFinalize() to ospray
    cerr << "warning: double init" << endl;
    }
}

//----------------------------------------------------------------------------
vtkOsprayWindowNode::~vtkOsprayWindowNode()
{
}

//----------------------------------------------------------------------------
void vtkOsprayWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsprayWindowNode::Render()
{
  vtkViewNodeCollection *renderers = this->GetChildren();
  vtkCollectionIterator *it = renderers->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkOsprayRendererNode *child =
      vtkOsprayRendererNode::SafeDownCast(it->GetCurrentObject());
    child->Render();
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkOsprayWindowNode::RenderSelf()
{
}

//----------------------------------------------------------------------------
void vtkOsprayWindowNode::PostRender()
{
  //composite all renderers framebuffers together
  unsigned char *rgba = new unsigned char[this->Size[0]*this->Size[1]*4];
  float *z = new float[this->Size[0]*this->Size[1]];

  vtkViewNodeCollection *renderers = this->GetChildren();
  vtkCollectionIterator *it = renderers->NewIterator();
  it->InitTraversal();

  int layer = 0;
  int count = 0;
  while (count < renderers->GetNumberOfItems())
    {
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
      {
      vtkOsprayRendererNode *child =
        vtkOsprayRendererNode::SafeDownCast(it->GetCurrentObject());
      if (child->GetLayer() == layer)
        {
        child->WriteLayer(rgba, z, this->Size[0], this->Size[1]);
        count++;
        }
      it->GoToNextItem();
      }
    layer++;
    }
  it->Delete();

  //show the result
  vtkRenderWindow *rwin = vtkRenderWindow::SafeDownCast(this->Renderable);
  rwin->SetZbufferData(0,  0, this->Size[0]-1, this->Size[1]-1, z);
  rwin->SetRGBACharPixelData( 0,  0, this->Size[0]-1, this->Size[1]-1,
                              rgba, 0, 0 );

  rwin->Frame(); //TODO: Why twice?
  rwin->Frame();
  delete[] rgba;
  delete[] z;
}
