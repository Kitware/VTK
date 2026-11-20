// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Shared scaffolding for the WebGPU volume tests: it pins the rendering backend, builds the
 * transfer functions the tests reuse, and reads frames back so a test can assert something about
 * what was drawn without needing a baseline image.
 */

#ifndef vtkWebGPUVolumeTestHelpers_h
#define vtkWebGPUVolumeTestHelpers_h

#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolumeProperty.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace vtkWebGPUVolumeTest
{

/**
 * Fail loudly when the factory handed back something other than the class under test, rather
 * than quietly passing while exercising a different backend.
 */
inline bool CheckClass(vtkObject* object, const char* expected)
{
  if (object == nullptr || std::string(object->GetClassName()) != expected)
  {
    std::cerr << "Expected a " << expected << " but got "
              << (object != nullptr ? object->GetClassName() : "nothing") << ".\n";
    return false;
  }
  return true;
}

/**
 * A frame, as RGB triples ordered row by row.
 */
struct Frame
{
  std::vector<unsigned char> Pixels;
  int Width = 0;
  int Height = 0;

  const unsigned char* At(int x, int y) const
  {
    return &this->Pixels[3 * (static_cast<std::size_t>(y) * this->Width + x)];
  }

  /// Pixels that differ from the background by more than \a tolerance per channel.
  std::size_t CountLit(const double background[3], int tolerance = 12) const
  {
    std::size_t count = 0;
    for (std::size_t i = 0; i + 2 < this->Pixels.size(); i += 3)
    {
      bool lit = false;
      for (int c = 0; c < 3; ++c)
      {
        const int reference = static_cast<int>(background[c] * 255.0 + 0.5);
        lit = lit || std::abs(static_cast<int>(this->Pixels[i + c]) - reference) > tolerance;
      }
      count += lit ? 1 : 0;
    }
    return count;
  }
};

inline Frame Capture(vtkRenderWindow* window)
{
  Frame frame;
  const int* size = window->GetSize();
  frame.Width = size[0];
  frame.Height = size[1];

  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetPixelData(0, 0, frame.Width - 1, frame.Height - 1, /*front=*/1, pixels);
  frame.Pixels.assign(pixels->GetPointer(0),
    pixels->GetPointer(0) + static_cast<std::size_t>(pixels->GetNumberOfValues()));
  return frame;
}

/**
 * Largest per channel difference between two frames of the same size.
 */
inline int LargestDifference(const Frame& left, const Frame& right)
{
  if (left.Pixels.size() != right.Pixels.size())
  {
    return 255;
  }
  int largest = 0;
  for (std::size_t i = 0; i < left.Pixels.size(); ++i)
  {
    largest = std::max(
      largest, std::abs(static_cast<int>(left.Pixels[i]) - static_cast<int>(right.Pixels[i])));
  }
  return largest;
}

/**
 * Mean per channel difference between two frames of the same size.
 */
inline double MeanDifference(const Frame& left, const Frame& right)
{
  if (left.Pixels.size() != right.Pixels.size() || left.Pixels.empty())
  {
    return 255.0;
  }
  double total = 0.0;
  for (std::size_t i = 0; i < left.Pixels.size(); ++i)
  {
    total += std::abs(static_cast<int>(left.Pixels[i]) - static_cast<int>(right.Pixels[i]));
  }
  return total / left.Pixels.size();
}

/**
 * A property that maps \a range onto a blue to yellow ramp of growing opacity.
 */
inline vtkSmartPointer<vtkVolumeProperty> MakeProperty(
  const double range[2], const double rgb[3] = nullptr)
{
  vtkNew<vtkColorTransferFunction> color;
  if (rgb != nullptr)
  {
    color->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
    color->AddRGBPoint(range[1], rgb[0], rgb[1], rgb[2]);
  }
  else
  {
    color->AddRGBPoint(range[0], 0.0, 0.1, 0.6);
    color->AddRGBPoint(0.5 * (range[0] + range[1]), 0.1, 0.9, 0.3);
    color->AddRGBPoint(range[1], 1.0, 0.8, 0.1);
  }

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(range[0], 0.0);
  opacity->AddPoint(0.5 * (range[0] + range[1]), 0.05);
  opacity->AddPoint(range[1], 0.6);

  vtkSmartPointer<vtkVolumeProperty> property = vtkSmartPointer<vtkVolumeProperty>::New();
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetScalarOpacityUnitDistance(2.0);
  property->SetInterpolationTypeToLinear();
  return property;
}

} // namespace vtkWebGPUVolumeTest

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeTestHelpers.h
