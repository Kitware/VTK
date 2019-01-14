/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindowNode.h"

#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewNodeCollection.h"

//============================================================================
vtkStandardNewMacro(vtkWindowNode);

//----------------------------------------------------------------------------
vtkWindowNode::vtkWindowNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->ColorBuffer = vtkUnsignedCharArray::New();
  this->ZBuffer = vtkFloatArray::New();
}

//----------------------------------------------------------------------------
vtkWindowNode::~vtkWindowNode()
{
  this->ColorBuffer->Delete();
  this->ColorBuffer = 0;
  this->ZBuffer->Delete();
  this->ZBuffer = 0;
}

//----------------------------------------------------------------------------
void vtkWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkWindowNode::Build(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow *mine = vtkRenderWindow::SafeDownCast
      (this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetRenderers());
    this->RemoveUnusedNodes();
  }
}

//----------------------------------------------------------------------------
void vtkWindowNode::Synchronize(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow *mine = vtkRenderWindow::SafeDownCast
      (this->GetRenderable());
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
    int * sz = mine->GetSize();
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
  }
}
