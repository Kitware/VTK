// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWindowNode.h"

#include "vtkCollectionIterator.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRendererNode.h"
#include "vtkUnsignedCharArray.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWindowNode);

//------------------------------------------------------------------------------
vtkWindowNode::vtkWindowNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->ColorBuffer = vtkUnsignedCharArray::New();
  this->ZBuffer = vtkFloatArray::New();
}

//------------------------------------------------------------------------------
vtkWindowNode::~vtkWindowNode()
{
  this->ColorBuffer->Delete();
  this->ColorBuffer = nullptr;
  this->ZBuffer->Delete();
  this->ZBuffer = nullptr;
}

//------------------------------------------------------------------------------
void vtkWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWindowNode::Build(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow* mine = vtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetRenderers());
    this->RemoveUnusedNodes();
  }
}

//------------------------------------------------------------------------------
void vtkWindowNode::Synchronize(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow* mine = vtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    /*
      GetAAFrames()   vtkRenderWindow virtual
      GetActualSize() vtkWindow
      GetAlphaBitPlanes()     vtkRenderWindow virtual
      GetDoubleBuffer()       vtkWindow       virtual
      GetDPI()        vtkWindow       virtual
      GetFDFrames()   vtkRenderWindow virtual
      GetFullScreen() vtkRenderWindow virtual
      GetLineSmoothing()      vtkRenderWindow virtual
      GetMapped()     vtkWindow       virtual
      GetMTime()      vtkObject       virtual
      GetMultiSamples()       vtkRenderWindow virtual
      GetNeverRendered()      vtkRenderWindow virtual
      GetNumberOfLayers()     vtkRenderWindow virtual
      GetOffScreenRendering() vtkWindow       virtual
      GetPointSmoothing()     vtkRenderWindow virtual
      GetPolygonSmoothing()   vtkRenderWindow virtual
      GetPosition()   vtkWindow       virtual
      GetScreenSize()=0       vtkWindow       pure virtual
    */
    const int* sz = mine->GetSize();
    this->Size[0] = sz[0];
    this->Size[1] = sz[1];
    /*
      GetStereoType() vtkRenderWindow virtual
      GetSubFrames()  vtkRenderWindow virtual
      GetSwapBuffers()        vtkRenderWindow virtual
      GetTileScale()  vtkWindow       virtual
      GetTileViewport()       vtkWindow       virtual
      GetUseConstantFDOffsets()       vtkRenderWindow virtual
    */

    auto const& renderers = this->GetChildren();
    for (auto ren : renderers)
    {
      vtkRendererNode* child = vtkRendererNode::SafeDownCast(ren);
      child->SetSize(this->Size);
    }
  }
}
VTK_ABI_NAMESPACE_END
