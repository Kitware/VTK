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
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkOsprayRendererNode.h"
#include "vtkOsprayViewNodeFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"
#include <exception>

//============================================================================
vtkStandardNewMacro(vtkOsprayWindowNode);

//----------------------------------------------------------------------------
vtkOsprayWindowNode::vtkOsprayWindowNode()
{
  int ac = 1;
  const char* av[] = {"pvOSPRay\0"};
  try
    {
    ospInit(&ac, av);
    }
  catch (std::runtime_error &vtkNotUsed(e))
    {
    //todo: request addition of ospFinalize() to ospray
    //cerr << "warning: double init" << endl;
    }
  vtkOsprayViewNodeFactory *fac = vtkOsprayViewNodeFactory::New();
  this->SetMyFactory(fac);
  fac->Delete();
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
void vtkOsprayWindowNode::Render(bool prepass)
{
  if (!prepass)
    {
    //composite all renderers framebuffers together
    this->ColorBuffer->SetNumberOfComponents(4);
    this->ColorBuffer->SetNumberOfTuples(this->Size[0]*this->Size[1]);
    unsigned char *rgba = static_cast<unsigned char *>
      (this->ColorBuffer->GetVoidPointer(0));

    this->ZBuffer->SetNumberOfComponents(1);
    this->ZBuffer->SetNumberOfTuples(this->Size[0]*this->Size[1]);
    float *z = static_cast<float *>
      (this->ZBuffer->GetVoidPointer(0));

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
        vtkRenderer *ren = vtkRenderer::SafeDownCast(child->GetRenderable());
        if (ren->GetLayer() == layer)
          {
          child->WriteLayer(rgba, z, this->Size[0], this->Size[1], layer);
          count++;
          }
        it->GoToNextItem();
        }
      layer++;
      }
    it->Delete();
    }
}
