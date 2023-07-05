// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLCellGridMapper.h"

#include "vtkActor.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCellGridRenderRequest.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

class vtkOpenGLCellGridMapper::vtkInternals
{
public:
  explicit vtkInternals(vtkOpenGLCellGridMapper* Mapper);
  ~vtkInternals();
  vtkInternals(const vtkInternals&) = delete;
  void operator=(const vtkInternals&) = delete;

  std::unique_ptr<vtkGenericOpenGLResourceFreeCallback> ResourceCallback;
  vtkOpenGLCellGridMapper* OglCellGridMapper = nullptr;
  vtkNew<vtkOpenGLCellGridRenderRequest> RenderQuery;
};

vtkOpenGLCellGridMapper::vtkInternals::vtkInternals(vtkOpenGLCellGridMapper* mapper)
  : OglCellGridMapper(mapper)
{
  this->RenderQuery->SetMapper(mapper);
  using CallBackT = vtkOpenGLResourceFreeCallback<vtkOpenGLCellGridMapper>;
  this->ResourceCallback = std::unique_ptr<CallBackT>(
    new CallBackT(this->OglCellGridMapper, &vtkOpenGLCellGridMapper::ReleaseGraphicsResources));
}

vtkOpenGLCellGridMapper::vtkInternals::~vtkInternals()
{
  this->ResourceCallback->Release();
}

vtkStandardNewMacro(vtkOpenGLCellGridMapper);

vtkOpenGLCellGridMapper::vtkOpenGLCellGridMapper()
  : Internal(new vtkInternals(this))
{
  // We default to interpolating scalars before mapping
  // (because the GLSL shaders do this per fragment).
  // Currently, there is no other mode supported.
  this->InterpolateScalarsBeforeMapping = 1;

#ifndef NDEBUG
  // this->DebugOn(); // Uncomment this for informational messages during renders.
#endif
}

vtkOpenGLCellGridMapper::~vtkOpenGLCellGridMapper()
{
  delete this->Internal;
}

void vtkOpenGLCellGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLCellGridMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  if (!this->Internal->ResourceCallback->IsReleasing())
  {
    this->Internal->ResourceCallback->Release();
    return;
  }

  auto* cellGrid = this->GetInput();
  if (cellGrid)
  {
    this->Internal->RenderQuery->SetIsReleasingResources(true);
    this->Internal->RenderQuery->SetWindow(window);
    cellGrid->Query(this->Internal->RenderQuery);
  }
  this->Modified();
}

void vtkOpenGLCellGridMapper::ShallowCopy(vtkAbstractMapper*) {}

void vtkOpenGLCellGridMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  vtkDebugMacro(<< __func__);
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->Internal->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  auto* cellGrid = this->GetInput();
  if (cellGrid)
  {
    // TODO: Here, we hardwire a colormap for the vtkCellAttribute used for coloring.
    //       In practice, we need another vtkCellQuery for choosing the colormap to
    //       be consistent across all cell types (i.e., to get the field range correct).
    auto* colorAttribute = cellGrid->GetCellAttributeByName(this->GetArrayName());
    if (colorAttribute)
    {
      auto cmap = colorAttribute->GetColormap();
      if (!cmap)
      {
        // Create a cool-to-warm (blue to red) diverging colormap by default:
        vtkNew<vtkColorTransferFunction> ctf;
        ctf->SetVectorModeToMagnitude();
        ctf->SetColorSpaceToDiverging();
        ctf->AddRGBPoint(0.0, 59. / 255., 76. / 255., 192. / 255.);
        ctf->AddRGBPoint(0.5, 221. / 255., 221. / 255., 221. / 255.);
        ctf->AddRGBPoint(1.0, 180. / 255., 4. / 255., 38. / 255.);
        ctf->Build();
        colorAttribute->SetColormap(ctf);
        cmap = ctf;
      }
      // Now, if there is no colormap texture, make one from the colormap
      if (!this->LookupTable || this->LookupTable->GetMTime() < cmap->GetMTime())
      {
        this->SetLookupTable(cmap);
      }
      if (!this->ColorTextureMap ||
        this->ColorTextureMap->GetMTime() < this->LookupTable->GetMTime())
      {
        this->CreateColormapTexture(); // populate this->ColorTexture from this->LookupTable
      }
      // The RenderQuery responders can now call this->GetColorTextureMap()
      // and use it for color lookup.
    }

    // Render the cells using our render-query.
    this->Internal->RenderQuery->SetRenderer(ren);
    this->Internal->RenderQuery->SetActor(act);
    cellGrid->Query(this->Internal->RenderQuery);
  }
}

VTK_ABI_NAMESPACE_END
