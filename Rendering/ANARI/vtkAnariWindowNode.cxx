// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariWindowNode.h"

#include "vtkAnariProfiling.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariViewNodeFactory.h"

#include "vtkCollectionIterator.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkUnsignedCharArray.h"

#include <anari/anari_cpp.hpp>

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariWindowNode);

//----------------------------------------------------------------------------
vtkAnariWindowNode::vtkAnariWindowNode()
{
  vtkNew<vtkAnariViewNodeFactory> anariViewNodeFactory;
  this->SetMyFactory(anariViewNodeFactory);
}

//----------------------------------------------------------------------------
void vtkAnariWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariWindowNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariWindowNode::Render", vtkAnariProfiling::BROWN);

  if (!prepass)
  {
    int totalSize = this->Size[0] * this->Size[1];

    // composite all renderers framebuffers together
    this->ColorBuffer->SetNumberOfComponents(4);
    this->ColorBuffer->SetNumberOfTuples(totalSize);
    unsigned char* rgba = static_cast<unsigned char*>(this->ColorBuffer->GetVoidPointer(0));

    this->ZBuffer->SetNumberOfComponents(1);
    this->ZBuffer->SetNumberOfTuples(totalSize);
    float* z = static_cast<float*>(this->ZBuffer->GetVoidPointer(0));

    auto const& renderers = this->GetChildren();

    int layer = 0;
    int count = 0;
    while (count < static_cast<int>(renderers.size()))
    {
      for (auto node : renderers)
      {
        vtkAnariRendererNode* child = vtkAnariRendererNode::SafeDownCast(node);
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
