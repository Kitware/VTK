/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayWindowNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayWindowNode.h"

#include "vtkCollectionIterator.h"
#include "vtkFloatArray.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayViewNodeFactory.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewNodeCollection.h"

#include "RTWrapper/RTWrapper.h"

//============================================================================
vtkStandardNewMacro(vtkOSPRayWindowNode);

//----------------------------------------------------------------------------
vtkOSPRayWindowNode::vtkOSPRayWindowNode()
{
  vtkOSPRayPass::RTInit();
  vtkOSPRayViewNodeFactory* fac = vtkOSPRayViewNodeFactory::New();
  this->SetMyFactory(fac);
  fac->Delete();
}

//----------------------------------------------------------------------------
vtkOSPRayWindowNode::~vtkOSPRayWindowNode()
{
  vtkOSPRayPass::RTShutdown();
}

//----------------------------------------------------------------------------
void vtkOSPRayWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayWindowNode::Render(bool prepass)
{
  if (!prepass)
  {
    // composite all renderers framebuffers together
    this->ColorBuffer->SetNumberOfComponents(4);
    this->ColorBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    unsigned char* rgba = static_cast<unsigned char*>(this->ColorBuffer->GetVoidPointer(0));

    this->ZBuffer->SetNumberOfComponents(1);
    this->ZBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    float* z = static_cast<float*>(this->ZBuffer->GetVoidPointer(0));

    vtkViewNodeCollection* renderers = this->GetChildren();
    vtkCollectionIterator* it = renderers->NewIterator();
    it->InitTraversal();

    int layer = 0;
    int count = 0;
    while (count < renderers->GetNumberOfItems())
    {
      it->InitTraversal();
      while (!it->IsDoneWithTraversal())
      {
        vtkOSPRayRendererNode* child = vtkOSPRayRendererNode::SafeDownCast(it->GetCurrentObject());
        vtkRenderer* ren = vtkRenderer::SafeDownCast(child->GetRenderable());
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
