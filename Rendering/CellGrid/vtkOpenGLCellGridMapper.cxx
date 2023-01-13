// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLCellGridMapper.h"

#include "vtkActor.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridMapper.h"
#include "vtkCellGridRenderRequest.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkRenderer.h"
#include "vtkRenderingCellGrid.h"
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
  ~vtkInternals() = default;
  vtkInternals(const vtkInternals&) = delete;
  void operator=(const vtkInternals&) = delete;

  vtkNew<vtkCellGridRenderRequest> RenderQuery;
};

vtkOpenGLCellGridMapper::vtkInternals::vtkInternals(vtkOpenGLCellGridMapper* mapper)
{
  this->RenderQuery->SetMapper(mapper);
}

vtkStandardNewMacro(vtkOpenGLCellGridMapper);

vtkOpenGLCellGridMapper::vtkOpenGLCellGridMapper()
  : Internal(new vtkInternals(this))
{
  // std::cout << "Constructing " << this->GetObjectDescription() << std::endl;

  // We default to interpolating scalars before mapping
  // (because the GLSL shaders do this per fragment).
  // Currently, there is no other mode supported.
  this->InterpolateScalarsBeforeMapping = 1;

  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLCellGridMapper>(
    this, &vtkOpenGLCellGridMapper::ReleaseGraphicsResources);

#ifndef NDEBUG
  // this->DebugOn(); // Uncomment this for informational messages during renders.
#endif

  // Plugins are expected to register responders, but for the base functionality provided
  // by VTK itself, we use this object to register responders at construction.
  // Since the vtkCellGridMapper owns an instance of this request, the registration
  // is guaranteed to occur in time for the first render of cell types supported by VTK.
  vtkRenderingCellGrid::RegisterCellsAndResponders();
}

vtkOpenGLCellGridMapper::~vtkOpenGLCellGridMapper()
{
  // std::cout << "Destructing " << this->GetObjectDescription() << std::endl;

  // std::cout << "First deleting this->Internal" << std::endl;
  delete this->Internal;
  this->Internal = nullptr;

  if (this->ResourceCallback)
  {
    // std::cout << "Now triggering a release" << std::endl;
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
}

void vtkOpenGLCellGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLCellGridMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  // std::cout << "Inside vtkOpenGLCellGridMapper::ReleaseGraphicsResources -> " <<
  // this->GetObjectDescription() << std::endl;

  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  // If called from our own destructor (where we delete this->Internal), do nothing.
  if (!this->Internal)
  {
    // std::cout << "  called from our own destructor, exiting early." << std::endl;
    return;
  }

  auto* cellGrid = this->GetInput();

  if (cellGrid)
  {
    this->Internal->RenderQuery->SetIsReleasingResources(true);
    this->Internal->RenderQuery->SetWindow(window);
    // std::cout << "  issuing query to release resources" << std::endl;
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

  this->ResourceCallback->RegisterGraphicsResources(
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
