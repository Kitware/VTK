// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Reproduces unbounded growth of the per-context texture-buffer cache
// (vtkOpenGLArrayTextureBufferCache) when geometry is continually modified.
//
// The vertex-pulling mapper (vtkOpenGLLowMemoryPolyDataMapper) uploads its
// arrays through the per-context cache, keyed on the vtkDataArray pointer. Any
// workflow that keeps producing *new* arrays each frame by re-executing filters,
// re-generated polydata, interactive mesh edits inserts a new entry per array
// per frame. Nothing ever evicts old entries, and each entry holds a
// vtkSmartPointer to its source array, so both host and GPU memory grow with
// every update until the render window is destroyed. On WebAssembly builds this
// exhausts the heap and crashes the application.
//
// The test renders one actor whose polydata is replaced with a fresh deep copy
// (new array instances, same geometry) for a number of update cycles, and then
// requires the number of cached texture buffers to stay bounded rather than
// scale with the number of updates.

#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOpenGLArrayTextureBufferCache.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringScanner.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
// Number of "the user modified the geometry" cycles to simulate. Overridable
// with --updates N to demonstrate the crash: with --resolution large enough,
// the retained arrays exhaust the 4GB wasm32 heap and abort the application.
int NumUpdates = 64;

// Sphere tessellation. Overridable with --resolution R; each update retains
// O(R^2) points/normals/scalars/connectivity in the cache.
int Resolution = 32;

// Build a sphere (its source already provides point normals) with an elevation
// scalar array so points, normals, connectivity and colors all flow through the
// cache.
vtkSmartPointer<vtkPolyData> MakeSphere()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(Resolution);
  sphere->SetPhiResolution(Resolution);
  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(sphere->GetOutputPort());
  elevation->SetLowPoint(0.0, -0.5, 0.0);
  elevation->SetHighPoint(0.0, 0.5, 0.0);
  elevation->Update();
  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->DeepCopy(vtkPolyData::SafeDownCast(elevation->GetOutput()));
  return mesh;
}

std::size_t CachedTextureBufferCount(vtkRenderWindow* renderWindow)
{
  auto* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);
  if (!glRenderWindow || !glRenderWindow->GetArrayTextureBufferCache())
  {
    return 0;
  }
  return glRenderWindow->GetArrayTextureBufferCache()->GetNumberOfCachedTextureBuffers();
}

// Host-side KiB pinned by the cache's source-array smart pointers.
unsigned long CachedCPUMemoryKiB(vtkRenderWindow* renderWindow)
{
  auto* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);
  if (!glRenderWindow || !glRenderWindow->GetArrayTextureBufferCache())
  {
    return 0;
  }
  return glRenderWindow->GetArrayTextureBufferCache()->GetCPUMemorySize();
}

// Device-side KiB pinned by the cache's source-array smart pointers.
unsigned long CachedGPUMemoryKiB(vtkRenderWindow* renderWindow)
{
  auto* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);
  if (!glRenderWindow || !glRenderWindow->GetArrayTextureBufferCache())
  {
    return 0;
  }
  return static_cast<unsigned long>(
    vtkMath::Ceil(glRenderWindow->GetArrayTextureBufferCache()->GetCurrentCacheSize() / 1024.0));
}

// updatesDone == 0 means the warm-up render.
void PrintCacheStats(int updatesDone, vtkRenderWindow* renderWindow)
{
  std::cout << "Cached texture buffers after ";
  if (updatesDone == 0)
  {
    std::cout << "first render";
  }
  else
  {
    std::cout << updatesDone << " updates";
  }
  std::cout << ": " << CachedTextureBufferCount(renderWindow)
            << " (CPU memory: " << CachedCPUMemoryKiB(renderWindow) / 1024.0
            << " MiB|GPU memory: " << CachedGPUMemoryKiB(renderWindow) / 1024.0 << " MiB)\n";
}
} // namespace

int TestArrayTextureBufferCacheGrowth(int argc, char* argv[])
{
  for (int i = 1; i < argc - 1; ++i)
  {
    if (std::strcmp(argv[i], "--updates") == 0)
    {
      NumUpdates = vtk::scan_int<int>(argv[++i])->value();
    }
    else if (std::strcmp(argv[i], "--resolution") == 0)
    {
      Resolution = vtk::scan_int<int>(argv[++i])->value();
    }
  }
  std::cout << "Updates: " << NumUpdates << ", sphere resolution: " << Resolution << "\n";

  vtkSmartPointer<vtkPolyData> templateMesh = MakeSphere();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkOpenGLLowMemoryPolyDataMapper> mapper;
  // Shift/scale would wrap the points in a derived per-upload array and obscure
  // the counting below; the raw polydata arrays must reach the cache directly.
  mapper->SetVBOShiftScaleMethod(vtkPolyDataMapper::DISABLE_SHIFT_SCALE);
  mapper->SetInputData(templateMesh);
  mapper->SetScalarRange(0.0, 1.0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  // Warm-up render: the steady-state footprint of one live mapper.
  renderWindow->Render();
  const std::size_t baseline = CachedTextureBufferCount(renderWindow);
  PrintCacheStats(0, renderWindow);
  if (baseline == 0)
  {
    std::cerr << "ERROR: no texture buffers were cached; the vertex-pulling "
                 "mapper or cache was not exercised.\n";
    return EXIT_FAILURE;
  }

  // Continually "modify" the geometry: every cycle hands the mapper a fresh set
  // of vtkDataArray instances, exactly like a re-executing filter pipeline or
  // interactive mesh manipulation does.
  for (int i = 0; i < NumUpdates; ++i)
  {
    vtkSmartPointer<vtkPolyData> updatedMesh = vtkSmartPointer<vtkPolyData>::New();
    updatedMesh->DeepCopy(templateMesh);
    mapper->SetInputData(updatedMesh);
    renderWindow->Render();
    if ((i + 1) % 16 == 0)
    {
      PrintCacheStats(i + 1, renderWindow);
    }
  }

  const std::size_t finalCount = CachedTextureBufferCount(renderWindow);
  PrintCacheStats(NumUpdates, renderWindow);

  // A bounded cache keeps roughly one steady-state footprint (plus small slack
  // for entries in flight). The unbounded cache instead retains every array of
  // every update, growing by at least one entry per update.
  const std::size_t allowed = 2 * baseline + 4;
  if (finalCount > allowed)
  {
    std::cerr << "ERROR: cache grew from " << baseline << " to " << finalCount << " entries over "
              << NumUpdates << " geometry updates (allowed at most " << allowed
              << "). The texture buffer cache retains stale entries indefinitely.\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
