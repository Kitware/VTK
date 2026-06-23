// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Verifies the per-block partial re-upload path of vtkOpenGLArrayTextureBufferAdapter.
//
// When a composite dataset is rendered through the vertex-pulling batched mapper, every block's
// point/normal arrays are concatenated into one texture buffer. If a single block's point
// coordinates change (its point count unchanged), the adapter must re-transfer only that block's
// slice and leave the rest of the buffer in place. This test checks correctness of that path:
//
//   Window A: render two blocks, then translate one block's points and render again
//             (this triggers the layout-stable partial upload of just the moved block).
//   Window B: render two blocks with the same block already translated (a full upload).
//
// If the partial upload is correct, the incrementally-updated image A must match the
// freshly-built image B pixel-for-pixel.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
const double kBlockShift[3] = { 0.0, 1.5, 0.0 };

// Build a sphere (with normals) centered at the given location.
vtkSmartPointer<vtkPolyData> MakeSphere(double cx, double cy, double cz)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(cx, cy, cz);
  sphere->SetRadius(0.5);
  sphere->SetThetaResolution(48);
  sphere->SetPhiResolution(48);
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(sphere->GetOutputPort());
  normals->Update();
  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->DeepCopy(normals->GetOutput());
  return mesh;
}

// Two-block multiblock: block 0 at the origin, block 1 at (2,0,0), optionally pre-shifted.
vtkSmartPointer<vtkMultiBlockDataSet> MakeBlocks(bool shiftBlock1)
{
  vtkSmartPointer<vtkPolyData> block0 = MakeSphere(0.0, 0.0, 0.0);
  vtkSmartPointer<vtkPolyData> block1 = MakeSphere(2.0 + (shiftBlock1 ? kBlockShift[0] : 0.0),
    (shiftBlock1 ? kBlockShift[1] : 0.0), (shiftBlock1 ? kBlockShift[2] : 0.0));
  vtkSmartPointer<vtkMultiBlockDataSet> mbds = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mbds->SetNumberOfBlocks(2);
  mbds->SetBlock(0, block0);
  mbds->SetBlock(1, block1);
  return mbds;
}

void SetupRenderer(vtkRenderer* renderer)
{
  // A fixed camera so both windows render with identical parameters regardless of bounds.
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(1.0, 0.75, 0.0);
  camera->SetPosition(1.0, 0.75, 9.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  renderer->SetBackground(0.1, 0.1, 0.2);
}

vtkSmartPointer<vtkUnsignedCharArray> CaptureImage(vtkRenderWindow* renderWindow)
{
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(renderWindow);
  w2i->ReadFrontBufferOff();
  w2i->Update();
  auto scalars = vtkUnsignedCharArray::SafeDownCast(w2i->GetOutput()->GetPointData()->GetScalars());
  vtkSmartPointer<vtkUnsignedCharArray> copy = vtkSmartPointer<vtkUnsignedCharArray>::New();
  copy->DeepCopy(scalars);
  return copy;
}

// Largest per-channel difference between two captured images.
int MaxPixelDifference(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b)
{
  if (!a || !b || a->GetNumberOfValues() != b->GetNumberOfValues())
  {
    return 256;
  }
  int maxDiff = 0;
  const vtkIdType n = a->GetNumberOfValues();
  for (vtkIdType i = 0; i < n; ++i)
  {
    const int d = std::abs(static_cast<int>(a->GetValue(i)) - static_cast<int>(b->GetValue(i)));
    maxDiff = std::max(maxDiff, d);
  }
  return maxDiff;
}
} // namespace

int TestArrayTextureBufferPartialUpload(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // --- Window A: render, then translate block 1's points in place and render again. ---
  vtkSmartPointer<vtkMultiBlockDataSet> blocksA = MakeBlocks(/*shiftBlock1=*/false);
  vtkNew<vtkCompositePolyDataMapper> mapperA;
  mapperA->SetInputDataObject(blocksA);
  // Keep point arrays identical across frames so the moved block is detected by MTime, not
  // replaced by a per-frame shift/scale copy (which would force a full re-upload).
  mapperA->SetVBOShiftScaleMethod(vtkPolyDataMapper::DISABLE_SHIFT_SCALE);
  vtkNew<vtkActor> actorA;
  actorA->SetMapper(mapperA);

  vtkNew<vtkRenderer> rendererA;
  rendererA->AddActor(actorA);
  SetupRenderer(rendererA);
  vtkNew<vtkRenderWindow> windowA;
  windowA->SetSize(400, 300);
  windowA->SetMultiSamples(0);
  windowA->AddRenderer(rendererA);
  windowA->Render();

  // Mutate only block 1's coordinates (same point count) and re-render -> partial upload.
  auto block1 = vtkPolyData::SafeDownCast(blocksA->GetBlock(1));
  vtkPoints* pts = block1->GetPoints();
  for (vtkIdType i = 0; i < pts->GetNumberOfPoints(); ++i)
  {
    double p[3];
    pts->GetPoint(i, p);
    p[0] += kBlockShift[0];
    p[1] += kBlockShift[1];
    p[2] += kBlockShift[2];
    pts->SetPoint(i, p);
  }
  pts->Modified();
  block1->Modified();
  windowA->Render();
  vtkSmartPointer<vtkUnsignedCharArray> imageA = CaptureImage(windowA);

  // --- Window B: ground truth, block 1 already translated, single full upload. ---
  vtkSmartPointer<vtkMultiBlockDataSet> blocksB = MakeBlocks(/*shiftBlock1=*/true);
  vtkNew<vtkCompositePolyDataMapper> mapperB;
  mapperB->SetInputDataObject(blocksB);
  mapperB->SetVBOShiftScaleMethod(vtkPolyDataMapper::DISABLE_SHIFT_SCALE);
  vtkNew<vtkActor> actorB;
  actorB->SetMapper(mapperB);

  vtkNew<vtkRenderer> rendererB;
  rendererB->AddActor(actorB);
  SetupRenderer(rendererB);
  vtkNew<vtkRenderWindow> windowB;
  windowB->SetSize(400, 300);
  windowB->SetMultiSamples(0);
  windowB->AddRenderer(rendererB);
  windowB->Render();
  vtkSmartPointer<vtkUnsignedCharArray> imageB = CaptureImage(windowB);

  const int maxDiff = MaxPixelDifference(imageA, imageB);
  std::cout << "Max per-channel pixel difference (partial vs full upload): " << maxDiff << "\n";

  // The two images must be identical up to negligible rasterization noise. A real partial-upload
  // bug (stale or wrong slice) shifts the moved sphere and yields large differences.
  if (maxDiff > 2)
  {
    std::cerr << "ERROR: incremental (partial-upload) image does not match the freshly-built "
                 "image; the per-block partial upload is incorrect.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
