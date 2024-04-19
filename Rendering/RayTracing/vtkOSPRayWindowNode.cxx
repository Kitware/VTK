// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "RTWrapper/RTWrapper.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOSPRayWindowNode);

//------------------------------------------------------------------------------
vtkOSPRayWindowNode::vtkOSPRayWindowNode()
{
  vtkOSPRayPass::RTInit();
  vtkOSPRayViewNodeFactory* fac = vtkOSPRayViewNodeFactory::New();
  this->SetMyFactory(fac);
  fac->Delete();
}

//------------------------------------------------------------------------------
vtkOSPRayWindowNode::~vtkOSPRayWindowNode()
{
  vtkOSPRayPass::RTShutdown();
}

//------------------------------------------------------------------------------
void vtkOSPRayWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayWindowNode::Render(bool prepass)
{
  if (!vtkOSPRayPass::IsSupported())
  {
    static bool warned = false;
    if (!warned)
    {
      vtkWarningMacro(<< "Ignoring render request because OSPRay is not supported.");
      warned = true;
    }
    return;
  }

  if (!prepass)
  {
    // composite all renderers framebuffers together
    this->ColorBuffer->SetNumberOfComponents(4);
    this->ColorBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    unsigned char* rgba = static_cast<unsigned char*>(this->ColorBuffer->GetVoidPointer(0));

    this->ZBuffer->SetNumberOfComponents(1);
    this->ZBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    float* z = static_cast<float*>(this->ZBuffer->GetVoidPointer(0));

    auto const& renderers = this->GetChildren();

    int layer = 0;
    int count = 0;
    while (count < static_cast<int>(renderers.size()))
    {
      for (auto node : renderers)
      {
        vtkOSPRayRendererNode* child = vtkOSPRayRendererNode::SafeDownCast(node);
        vtkRenderer* ren = vtkRenderer::SafeDownCast(child->GetRenderable());
        if (ren->GetLayer() == layer)
        {
          child->WriteLayer(rgba, z, this->Size[0], this->Size[1], layer);
          count++;
        }
      }
      layer++;
    }
  }
}
VTK_ABI_NAMESPACE_END
