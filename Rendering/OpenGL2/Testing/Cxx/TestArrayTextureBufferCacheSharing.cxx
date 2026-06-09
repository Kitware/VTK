// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Verifies the per-context texture-buffer cache (vtkOpenGLArrayTextureBufferCache)
// dedupes geometry shared between several vertex-pulling mappers.
//
// The vertex-pulling mapper (vtkOpenGLLowMemoryPolyDataMapper, via
// vtkDrawTexturedElements) binds its point/normal/connectivity arrays as
// samplerBuffer texture objects. When several mappers render the *same* input
// polydata, the arrays that come straight off the polydata (points, normals)
// are the very same vtkDataArray instances, so the cache must store one texture
// buffer for each instead of one per mapper.
//
// The test renders one shared mesh through N mappers (each in its own renderer /
// sub-viewport) and, separately, N independent deep copies of that mesh through
// N more mappers. With caching, the shared scene must end up with strictly fewer
// cached texture buffers than the distinct scene -- the saving being at least one
// entry per extra mapper for the shared point coordinates alone.

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOpenGLArrayTextureBufferCache.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
constexpr int NumMappers = 4;

// Sub-viewport bounds for a 2x2 grid, indexed by mapper.
const std::array<std::array<double, 4>, NumMappers> Viewports = { {
  { 0.0, 0.0, 0.5, 0.5 },
  { 0.5, 0.0, 1.0, 0.5 },
  { 0.0, 0.5, 0.5, 1.0 },
  { 0.5, 0.5, 1.0, 1.0 },
} };

// Build a sphere with point normals to exercise more than one shared array.
vtkSmartPointer<vtkPolyData> MakeSphere()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(sphere->GetOutputPort());
  normals->Update();
  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->DeepCopy(normals->GetOutput());
  return mesh;
}

// Fill a render window with NumMappers renderers laid out in a 2x2 grid, each
// drawing the polydata returned by meshForMapper(i) through its own
// low-memory (vertex-pulling) mapper and actor.
template <typename MeshForMapper>
void BuildScene(vtkRenderWindow* renderWindow, MeshForMapper meshForMapper)
{
  for (int i = 0; i < NumMappers; ++i)
  {
    vtkNew<vtkOpenGLLowMemoryPolyDataMapper> mapper;
    // Keep the points array identical across mappers: shift/scale would replace
    // it with a per-mapper copy and defeat the very sharing under test.
    mapper->SetVBOShiftScaleMethod(vtkPolyDataMapper::DISABLE_SHIFT_SCALE);
    mapper->SetInputData(meshForMapper(i));

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.9, 0.6, 0.3);

    vtkNew<vtkRenderer> renderer;
    const auto& vp = Viewports[i];
    renderer->SetViewport(vp[0], vp[1], vp[2], vp[3]);
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renderWindow->AddRenderer(renderer);
  }
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
} // namespace

int TestArrayTextureBufferCacheSharing(int argc, char* argv[])
{
  // One mesh shared by every mapper: the points/normals arrays are the same
  // vtkDataArray instances, so the cache should dedupe them.
  vtkSmartPointer<vtkPolyData> sharedMesh = MakeSphere();

  vtkNew<vtkRenderWindow> sharedWindow;
  sharedWindow->SetSize(400, 400);
  sharedWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(sharedWindow);
  BuildScene(sharedWindow, [&](int) { return sharedMesh.Get(); });
  sharedWindow->Render();
  const std::size_t sharedCount = CachedTextureBufferCount(sharedWindow);

  // A second window where every mapper owns an independent deep copy: no array is
  // shared, so the cache holds one set of texture buffers per mapper.
  std::array<vtkSmartPointer<vtkPolyData>, NumMappers> distinctMeshes;
  for (int i = 0; i < NumMappers; ++i)
  {
    distinctMeshes[i] = vtkSmartPointer<vtkPolyData>::New();
    distinctMeshes[i]->DeepCopy(sharedMesh);
  }
  vtkNew<vtkRenderWindow> distinctWindow;
  distinctWindow->SetSize(400, 400);
  distinctWindow->SetMultiSamples(0);
  BuildScene(distinctWindow, [&](int i) { return distinctMeshes[i].Get(); });
  distinctWindow->Render();
  const std::size_t distinctCount = CachedTextureBufferCount(distinctWindow);

  std::cout << "Cached texture buffers (shared mesh):   " << sharedCount << "\n";
  std::cout << "Cached texture buffers (distinct mesh): " << distinctCount << "\n";

  bool ok = true;
  if (sharedCount == 0)
  {
    std::cerr << "ERROR: the shared scene cached no texture buffers; the "
                 "vertex-pulling mapper or cache was not exercised.\n";
    ok = false;
  }
  // Each of the (NumMappers - 1) extra mappers in the distinct scene must add at
  // least the shared point-coordinate buffer that the shared scene dedupes away.
  if (distinctCount < sharedCount + (NumMappers - 1))
  {
    std::cerr << "ERROR: expected the distinct-mesh scene to hold at least "
              << (sharedCount + (NumMappers - 1)) << " cached texture buffers (sharedCount + "
              << (NumMappers - 1) << "), but it held " << distinctCount
              << ". The cache is not deduping shared arrays.\n";
    ok = false;
  }

  if (!ok)
  {
    return EXIT_FAILURE;
  }

  const int retVal = vtkRegressionTestImage(sharedWindow);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    interactor->Start();
  }
  // NO_VALID: there is no committed baseline; the cache assertions above are the
  // real verification. Treat only an explicit image failure as fatal.
  if (retVal == vtkTesting::FAILED)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
