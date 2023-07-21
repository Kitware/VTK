/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeSurfaceLICMapper.h"

#include "vtk_glew.h"

#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeSurfaceLICMapperDelegator.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPainterCommunicator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <algorithm>
#include <sstream>

#include "vtkSurfaceLICInterface.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCompositeSurfaceLICMapper);
//------------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::vtkCompositeSurfaceLICMapper() = default;

//------------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::~vtkCompositeSurfaceLICMapper() = default;

vtkCompositePolyDataMapperDelegator* vtkCompositeSurfaceLICMapper::CreateADelegator()
{
  return vtkCompositeSurfaceLICMapperDelegator::New();
}

//------------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.

void vtkCompositeSurfaceLICMapper::Render(vtkRenderer* ren, vtkActor* actor)
{
  this->LICInterface->ValidateContext(ren);

  this->LICInterface->UpdateCommunicator(ren, actor, this->GetInputDataObject(0, 0));

  vtkPainterCommunicator* comm = this->LICInterface->GetCommunicator();

  if (comm->GetIsNull())
  {
    // other rank's may have some visible data but we
    // have none and should not participate further
    return;
  }

  // do we have vectors? Need a leaf node to know
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
  bool haveVectors = true;
  if (input)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
      vtkSmartPointer<vtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataObject* dso = iter->GetCurrentDataObject();
      vtkPolyData* pd = vtkPolyData::SafeDownCast(dso);
      if (pd && pd->GetPoints())
      {
        haveVectors = haveVectors && (this->GetInputArrayToProcess(0, pd) != nullptr);
      }
    }
  }
  else
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
    if (pd && pd->GetPoints())
    {
      haveVectors = (this->GetInputArrayToProcess(0, pd) != nullptr);
    }
  }

  this->LICInterface->SetHasVectors(haveVectors);

  if (!this->LICInterface->CanRenderSurfaceLIC(actor))
  {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::Render(ren, actor);
    return;
  }

  // Before start rendering LIC, capture some essential state so we can restore
  // it.
  vtkOpenGLRenderWindow* rw = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = rw->GetState();
  vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  vtkOpenGLState::ScopedglEnableDisable cfsaver(ostate, GL_CULL_FACE);

  vtkNew<vtkOpenGLFramebufferObject> fbo;
  fbo->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  ostate->PushFramebufferBindings();

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->LICInterface->InitializeResources();

  // draw the geometry
  this->LICInterface->PrepareForGeometry();

  this->Superclass::Render(ren, actor);

  this->LICInterface->CompletedGeometry();

  // Disable cull face to make sure geometry won't be culled again
  ostate->vtkglDisable(GL_CULL_FACE);

  // --------------------------------------------- composite vectors for parallel LIC
  this->LICInterface->GatherVectors();

  // ------------------------------------------- LIC on screen
  this->LICInterface->ApplyLIC();

  // ------------------------------------------- combine scalar colors + LIC
  this->LICInterface->CombineColorsAndLIC();

  // ----------------------------------------------- depth test and copy to screen
  this->LICInterface->CopyToScreen();

  ostate->PopFramebufferBindings();
}
VTK_ABI_NAMESPACE_END
