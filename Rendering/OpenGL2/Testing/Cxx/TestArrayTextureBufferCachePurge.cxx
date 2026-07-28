// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Verifies the eviction policy of the per-context texture-buffer cache
// (vtkOpenGLArrayTextureBufferCache):
//
// 1. Entries whose mapper was destroyed but whose arrays only the cache still
//    owns (mapper-derived buffers) are orphan-swept on the next render.
// 2. Pinned entries (GL objects referenced by a live mapper) survive even when
//    the cache is far over budget.
// 3. Unpinned entries whose arrays are still alive elsewhere are evicted
//    least-recently-used first until the byte budget is met.
// 4. Entries evicted for budget reasons are transparently re-created and
//    re-uploaded when a mapper requests their arrays again, while retained
//    entries are pure cache hits.
//
// The unbounded-growth reproduction (continually modified geometry) is covered
// separately by TestArrayTextureBufferCacheGrowth.
//
// Terminology used below: each mapper contributes "shared" entries (arrays
// owned by the polydata: points, normals, connectivity) and "derived" entries
// (arrays the mapper itself builds, e.g. vertex-id buffers). Derived arrays
// die with their mapper and become orphans; shared arrays stay warm cache as
// long as the polydata lives.

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOpenGLArrayTextureBufferCache.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
constexpr int NumMeshes = 4;

// Sub-viewport bounds for a 2x2 grid, indexed by mesh.
const std::array<std::array<double, 4>, NumMeshes> Viewports = { {
  { 0.0, 0.0, 0.5, 0.5 },
  { 0.5, 0.0, 1.0, 0.5 },
  { 0.0, 0.5, 0.5, 1.0 },
  { 0.5, 0.5, 1.0, 1.0 },
} };

vtkSmartPointer<vtkPolyData> MakeSphere()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);
  sphere->Update();
  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->DeepCopy(sphere->GetOutput());
  return mesh;
}

// One renderer + low-memory mapper + actor drawing the given mesh in the given
// sub-viewport; keeps handles so the scene can be dismantled piecewise.
struct SceneSlot
{
  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkOpenGLLowMemoryPolyDataMapper> Mapper;
};

SceneSlot AddSlot(vtkRenderWindow* renderWindow, vtkPolyData* mesh, int viewportIndex)
{
  SceneSlot slot;
  slot.Mapper = vtkSmartPointer<vtkOpenGLLowMemoryPolyDataMapper>::New();
  // Keep the mesh arrays reaching the cache directly (no derived shift/scale copies).
  slot.Mapper->SetVBOShiftScaleMethod(vtkPolyDataMapper::DISABLE_SHIFT_SCALE);
  slot.Mapper->SetInputData(mesh);

  vtkNew<vtkActor> actor;
  actor->SetMapper(slot.Mapper);

  slot.Renderer = vtkSmartPointer<vtkRenderer>::New();
  const auto& vp = Viewports[viewportIndex];
  slot.Renderer->SetViewport(vp[0], vp[1], vp[2], vp[3]);
  slot.Renderer->AddActor(actor);
  slot.Renderer->ResetCamera();
  renderWindow->AddRenderer(slot.Renderer);
  return slot;
}

void RemoveSlot(vtkRenderWindow* renderWindow, SceneSlot& slot)
{
  slot.Mapper->ReleaseGraphicsResources(renderWindow);
  renderWindow->RemoveRenderer(slot.Renderer);
  slot = SceneSlot{};
}
} // namespace

int TestArrayTextureBufferCachePurge(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  // Four identical-but-distinct meshes so every slot owns an equal set of
  // cache entries.
  std::array<vtkSmartPointer<vtkPolyData>, NumMeshes> meshes;
  std::array<SceneSlot, NumMeshes> slots;
  for (int i = 0; i < NumMeshes; ++i)
  {
    meshes[i] = MakeSphere();
    slots[i] = AddSlot(renderWindow, meshes[i], i);
  }
  renderWindow->Render();

  auto* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get());
  auto* cache = glRenderWindow ? glRenderWindow->GetArrayTextureBufferCache() : nullptr;
  if (!cache)
  {
    std::cerr << "ERROR: no texture buffer cache on the render window.\n";
    return EXIT_FAILURE;
  }
  const std::size_t initialCount = cache->GetNumberOfCachedTextureBuffers();
  const std::size_t initialBytes = cache->GetCurrentCacheSize();
  std::cout << "Initial: " << initialCount << " entries, " << initialBytes << " bytes\n";
  if (initialCount == 0 || initialCount % NumMeshes != 0 || initialBytes % NumMeshes != 0)
  {
    std::cerr << "ERROR: expected a non-empty cache with " << NumMeshes
              << " equal per-mesh entry sets, got " << initialCount << " entries / " << initialBytes
              << " bytes.\n";
    return EXIT_FAILURE;
  }
  const std::size_t entriesPerMesh = initialCount / NumMeshes;
  const std::size_t bytesPerMesh = initialBytes / NumMeshes;

  // Measure how many entries are mapper-derived: a second mapper on an
  // already-cached mesh hits the shared entries and only adds its own derived
  // buffers.
  SceneSlot extraSlot = AddSlot(renderWindow, meshes[NumMeshes - 1], 0);
  renderWindow->Render();
  const std::size_t derivedPerMapper = cache->GetNumberOfCachedTextureBuffers() - initialCount;
  std::cout << "Derived (per-mapper) entries: " << derivedPerMapper
            << ", shared (per-mesh): " << entriesPerMesh - derivedPerMapper << "\n";
  if (derivedPerMapper >= entriesPerMesh)
  {
    std::cerr << "ERROR: expected at least one shared polydata-owned entry per mesh.\n";
    return EXIT_FAILURE;
  }

  // 1. Orphan sweep: destroying the extra mapper leaves its derived arrays
  //    owned solely by the cache; the next render must reclaim them.
  RemoveSlot(renderWindow, extraSlot);
  renderWindow->Render();
  std::cout << "After removing the extra mapper: " << cache->GetNumberOfCachedTextureBuffers()
            << " entries\n";
  if (cache->GetNumberOfCachedTextureBuffers() != initialCount)
  {
    std::cerr << "ERROR: orphan sweep did not reclaim the destroyed mapper's derived entries.\n";
    return EXIT_FAILURE;
  }

  // 2. Pinned entries survive an over-budget cache: every entry is referenced
  //    by a live mapper, so a 1-byte budget must evict nothing.
  cache->SetMaximumCacheSize(1);
  renderWindow->Render();
  std::cout << "After render with 1-byte budget (all pinned): "
            << cache->GetNumberOfCachedTextureBuffers() << " entries\n";
  if (cache->GetNumberOfCachedTextureBuffers() != initialCount)
  {
    std::cerr << "ERROR: over-budget purge evicted pinned entries (" << initialCount << " -> "
              << cache->GetNumberOfCachedTextureBuffers() << ").\n";
    return EXIT_FAILURE;
  }

  // Unpin meshes 0..2 with the budget disabled: only their derived entries are
  // orphaned; their shared entries stay as warm cache (the polydata live on in
  // `meshes`).
  cache->SetMaximumCacheSize(0);
  for (int i = 0; i < 3; ++i)
  {
    RemoveSlot(renderWindow, slots[i]);
  }
  renderWindow->Render();
  const std::size_t afterUnpinCount = cache->GetNumberOfCachedTextureBuffers();
  const std::size_t afterUnpinBytes = cache->GetCurrentCacheSize();
  std::cout << "After unpinning meshes 0-2 (no budget): " << afterUnpinCount << " entries, "
            << afterUnpinBytes << " bytes\n";
  if (afterUnpinCount != initialCount - 3 * derivedPerMapper)
  {
    std::cerr << "ERROR: expected only the three destroyed mappers' derived entries to be "
                 "orphan-swept ("
              << initialCount - 3 * derivedPerMapper << " entries), got " << afterUnpinCount
              << ".\n";
    return EXIT_FAILURE;
  }

  // 3. LRU eviction: allow room for everything except ~1.5 unpinned shared
  //    sets. Meshes 0 and 1 are least recently used, so mesh 0's shared
  //    entries must go entirely and mesh 2's must survive entirely (the
  //    eviction stops inside mesh 1).
  const std::size_t sharedBytesPerMesh = (afterUnpinBytes - bytesPerMesh) / 3;
  cache->SetMaximumCacheSize(afterUnpinBytes - sharedBytesPerMesh - sharedBytesPerMesh / 2);
  renderWindow->Render();
  const std::size_t afterBudgetCount = cache->GetNumberOfCachedTextureBuffers();
  const std::size_t afterBudgetBytes = cache->GetCurrentCacheSize();
  std::cout << "After ~1.5-shared-set budget: " << afterBudgetCount << " entries, "
            << afterBudgetBytes << " bytes (budget " << cache->GetMaximumCacheSize() << ")\n";
  if (afterBudgetBytes > cache->GetMaximumCacheSize())
  {
    std::cerr << "ERROR: cache size " << afterBudgetBytes << " exceeds the budget "
              << cache->GetMaximumCacheSize() << " with unpinned entries available to evict.\n";
    return EXIT_FAILURE;
  }
  if (afterBudgetCount >= afterUnpinCount)
  {
    std::cerr << "ERROR: the budget evicted nothing.\n";
    return EXIT_FAILURE;
  }

  // 4a. Mesh 2 (most recently used unpinned) must have been retained: re-adding
  //     a mapper for it re-creates only that mapper's derived entries.
  cache->SetMaximumCacheSize(0);
  const std::size_t beforeMesh2 = cache->GetNumberOfCachedTextureBuffers();
  slots[2] = AddSlot(renderWindow, meshes[2], 2);
  renderWindow->Render();
  const std::size_t growthMesh2 = cache->GetNumberOfCachedTextureBuffers() - beforeMesh2;
  std::cout << "Growth on re-adding retained mesh 2: " << growthMesh2 << " entries\n";
  if (growthMesh2 != derivedPerMapper)
  {
    std::cerr << "ERROR: expected the LRU-retained mesh 2 to be a shared-entry cache hit (growth "
              << derivedPerMapper << "), got growth " << growthMesh2 << ".\n";
    return EXIT_FAILURE;
  }

  // 4b. Mesh 0 (least recently used) must have been evicted: re-adding a
  //     mapper re-creates its shared entries and the mapper's derived entries.
  const std::size_t beforeMesh0 = cache->GetNumberOfCachedTextureBuffers();
  slots[0] = AddSlot(renderWindow, meshes[0], 0);
  renderWindow->Render();
  const std::size_t growthMesh0 = cache->GetNumberOfCachedTextureBuffers() - beforeMesh0;
  std::cout << "Growth on re-adding evicted mesh 0: " << growthMesh0 << " entries\n";
  if (growthMesh0 != entriesPerMesh)
  {
    std::cerr << "ERROR: expected the evicted mesh 0 to fully re-upload (growth " << entriesPerMesh
              << "), got growth " << growthMesh0 << ".\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
