// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test that large images do not cause integer overflow in texture allocation.
// This test creates a large image (dimensions that would overflow 32-bit integer
// when multiplied) and verifies it can be rendered correctly.
//
// The command line arguments are:
// -I        => run in interactive mode
// -B        => run in benchmark mode (multiple render iterations with timing)

#include "vtkRegressionTestImage.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>

int TestImageSliceMapperLargeImage(int argc, char* argv[])
{
  // Check for benchmark mode
  bool benchmarkMode = false;
  int numIterations = 5;
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-B") == 0)
    {
      benchmarkMode = true;
    }
  }

  // Create a large image that would cause integer overflow if dimensions
  // are multiplied as 32-bit integers. For example, 25000 x 25000 x 4 bytes
  // = 2,500,000,000 bytes which exceeds the signed integer byte limit (2,147,483,647).
  int width = 25000;
  int height = 25000;

  auto imageAllocStart = std::chrono::high_resolution_clock::now();

  vtkNew<vtkImageData> image;
  image->SetDimensions(width, height, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  // Fill the image with a simple checkered pattern
  int squareSize = 2500;
  for (int y = 0; y < height; ++y)
  {
    unsigned char* row = static_cast<unsigned char*>(image->GetScalarPointer(0, y, 0));
    for (int x = 0; x < width; ++x)
    {
      int checker = ((x / squareSize) + (y / squareSize)) % 2;
      row[x] = checker ? 255 : 0;
    }
  }

  auto imageAllocEnd = std::chrono::high_resolution_clock::now();
  double imageAllocTime =
    std::chrono::duration<double, std::milli>(imageAllocEnd - imageAllocStart).count();

  if (benchmarkMode)
  {
    std::cout << "Image allocation and fill time: " << imageAllocTime << " ms" << std::endl;
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 301);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.2, 0.4);
  renWin->AddRenderer(renderer);

  vtkNew<vtkImageSliceMapper> imageMapper;
  imageMapper->SetInputData(image);

  vtkNew<vtkImageSlice> imageSlice;
  imageSlice->SetMapper(imageMapper);
  renderer->AddViewProp(imageSlice);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  renderer->ResetCamera();

  // First render - includes texture upload (cold start)
  auto firstRenderStart = std::chrono::high_resolution_clock::now();
  renWin->Render();
  auto firstRenderEnd = std::chrono::high_resolution_clock::now();
  double firstRenderTime =
    std::chrono::duration<double, std::milli>(firstRenderEnd - firstRenderStart).count();

  if (benchmarkMode)
  {
    std::cout << "First render time (includes texture upload): " << firstRenderTime << " ms"
              << std::endl;

    // First test: Render WITHOUT forcing texture re-upload (cached texture path)
    std::cout << "\n--- Cached Texture Renders (no Modified() call) ---" << std::endl;
    std::cout << "Running " << numIterations << " render iterations..." << std::endl;
    double cachedTotalTime = 0.0;
    for (int i = 0; i < numIterations; ++i)
    {
      camera->Azimuth(1); // Slightly change view to avoid trivial caching
      renderer->ResetCamera();
      auto iterStart = std::chrono::high_resolution_clock::now();
      renWin->Render();
      auto iterEnd = std::chrono::high_resolution_clock::now();
      double iterTime = std::chrono::duration<double, std::milli>(iterEnd - iterStart).count();
      cachedTotalTime += iterTime;
      std::cout << "  Cached render " << (i + 1) << ": " << iterTime << " ms" << std::endl;
    }
    double avgCachedTime = cachedTotalTime / numIterations;
    std::cout << "  Average cached render: " << avgCachedTime << " ms" << std::endl;

    // Second test: Render WITH forced texture re-upload
    std::cout << "\n--- Forced Texture Re-upload (with Modified() call) ---" << std::endl;
    std::cout << "Running " << numIterations << " render iterations..." << std::endl;

    double totalTime = 0.0;
    double minTime = std::numeric_limits<double>::max();
    double maxTime = 0.0;

    for (int i = 0; i < numIterations; ++i)
    {
      // Modify the mapper to force texture re-upload
      imageMapper->Modified();

      renderer->ResetCamera();
      auto iterStart = std::chrono::high_resolution_clock::now();
      renWin->Render();
      auto iterEnd = std::chrono::high_resolution_clock::now();

      double iterTime = std::chrono::duration<double, std::milli>(iterEnd - iterStart).count();
      totalTime += iterTime;
      minTime = std::min(minTime, iterTime);
      maxTime = std::max(maxTime, iterTime);

      std::cout << "  Iteration " << (i + 1) << ": " << iterTime << " ms" << std::endl;
    }

    double avgTime = totalTime / numIterations;
    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "Image size: " << width << " x " << height << " ("
              << (static_cast<double>(width) * height / 1e6) << " megapixels)" << std::endl;
    std::cout << "First render (cold): " << firstRenderTime << " ms" << std::endl;
    std::cout << "Cached render (avg): " << avgCachedTime << " ms" << std::endl;
    std::cout << "Forced re-upload (avg): " << avgTime << " ms" << std::endl;
    std::cout << "Min re-upload time:  " << minTime << " ms" << std::endl;
    std::cout << "Max re-upload time:  " << maxTime << " ms" << std::endl;
    std::cout << "===========================" << std::endl;
  }

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
