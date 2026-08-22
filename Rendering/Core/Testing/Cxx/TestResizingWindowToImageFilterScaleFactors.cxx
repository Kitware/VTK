// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkResizingWindowToImageFilter.h"

#include <iostream>
#include <vector>

namespace
{
// GetScaleFactorsAndSize is protected because it is an implementation detail of
// RequestData. It is however the entire decision logic of the filter, so expose
// it here to exercise it without a render window.
class vtkScaleFactorProbe : public vtkResizingWindowToImageFilter
{
public:
  static vtkScaleFactorProbe* New();
  vtkTypeMacro(vtkScaleFactorProbe, vtkResizingWindowToImageFilter);

  using vtkResizingWindowToImageFilter::GetScaleFactorsAndSize;

protected:
  vtkScaleFactorProbe() = default;
  ~vtkScaleFactorProbe() override = default;

private:
  vtkScaleFactorProbe(const vtkScaleFactorProbe&) = delete;
  void operator=(const vtkScaleFactorProbe&) = delete;
};

struct Case
{
  const char* Description;
  int RequestedSize[2];
  int SizeLimit;
  int ExpectedSize[2];
  int ExpectedScale[2];
  bool ExpectedApproximate;
};
}
vtkStandardNewMacro(::vtkScaleFactorProbe);

int TestResizingWindowToImageFilterScaleFactors(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // The requested size is rendered directly when it fits within the size limit.
  // Otherwise the filter renders a smaller image and magnifies it by an integer
  // scale factor per axis, preferring a factor that preserves the aspect ratio.
  // When no exact factor exists the requested size can only be approximated.
  const std::vector<Case> cases = {
    { "fits within the limit", { 320, 240 }, 4000, { 320, 240 }, { 1, 1 }, false },
    { "exactly at the limit", { 200, 200 }, 200, { 200, 200 }, { 1, 1 }, false },
    { "common factor, aspect ratio preserved", { 320, 240 }, 200, { 160, 120 }, { 2, 2 }, false },
    { "factor per axis, aspect ratio not preserved", { 625, 512 }, 200, { 125, 128 }, { 5, 4 },
      false },
    { "only one axis exceeds the limit", { 625, 128 }, 200, { 125, 128 }, { 5, 1 }, false },
    { "one axis has no usable factor", { 403, 100 }, 200, { 134, 100 }, { 3, 1 }, true },
    { "neither axis has a usable factor", { 401, 403 }, 200, { 133, 134 }, { 3, 3 }, true },
  };

  int status = EXIT_SUCCESS;
  vtkNew<vtkScaleFactorProbe> probe;
  for (const Case& testCase : cases)
  {
    probe->SetSizeLimit(testCase.SizeLimit);

    int size[2] = { 0, 0 };
    int scale[2] = { 0, 0 };
    bool approximate = false;
    probe->GetScaleFactorsAndSize(testCase.RequestedSize, size, scale, &approximate);

    if (size[0] != testCase.ExpectedSize[0] || size[1] != testCase.ExpectedSize[1] ||
      scale[0] != testCase.ExpectedScale[0] || scale[1] != testCase.ExpectedScale[1] ||
      approximate != testCase.ExpectedApproximate)
    {
      std::cerr << "ERROR: " << testCase.Description << ": requested " << testCase.RequestedSize[0]
                << 'x' << testCase.RequestedSize[1] << " with a limit"
                << " of " << testCase.SizeLimit << ", expected size " << testCase.ExpectedSize[0]
                << 'x' << testCase.ExpectedSize[1] << " scaled by " << testCase.ExpectedScale[0]
                << 'x' << testCase.ExpectedScale[1] << " (approximate "
                << testCase.ExpectedApproximate << "), got size " << size[0] << 'x' << size[1]
                << " scaled by " << scale[0] << 'x' << scale[1] << " (approximate " << approximate
                << ")" << std::endl;
      status = EXIT_FAILURE;
      continue;
    }

    // The magnified image must never exceed the requested size, and it must
    // match it exactly unless the filter reported an approximation.
    const int actual[2] = { size[0] * scale[0], size[1] * scale[1] };
    if (actual[0] > testCase.RequestedSize[0] || actual[1] > testCase.RequestedSize[1] ||
      (!approximate &&
        (actual[0] != testCase.RequestedSize[0] || actual[1] != testCase.RequestedSize[1])))
    {
      std::cerr << "ERROR: " << testCase.Description << ": size times scale is " << actual[0] << 'x'
                << actual[1] << " which is inconsistent with the requested "
                << testCase.RequestedSize[0] << 'x' << testCase.RequestedSize[1] << " (approximate "
                << approximate << ")" << std::endl;
      status = EXIT_FAILURE;
    }
  }

  return status;
}
