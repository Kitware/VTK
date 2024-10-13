// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiBaselineRegressionTest_h
#define vtkMultiBaselineRegressionTest_h

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkTesting.h"

#include "vtksys/SystemTools.hxx"

#include <string.h>

/**\brief Run a regression test with an explicitly-provided image filename.
 *
 * Unlike the traditional C++ image-based test macro (vtkRegressionTestImage),
 * this templated function accepts the name of a baseline image.
 * It uses the existing vtkTesting infrastructure to expand the image name
 * into a full path by replacing the implied filename component of the valid
 * image (specified with "-V" on the command line) with the given \a img
 * value. The directory portion of the valid image path preceding is untouched.
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
int RegressionTestWithImageName(
  int argc, char* argv[], T* rw, const std::string& img, double thresh = 10.)
{
  vtkNew<vtkTesting> testing;
  bool isImgPath = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-V"))
    {
      isImgPath = true;
    }
    else if (isImgPath)
    {
      isImgPath = false;
      std::vector<std::string> components;
      std::string originalImage = argv[i];
      vtksys::SystemTools::SplitPath(originalImage, components);
      // Substitute image filename for last component;
      components.back() = img;
      std::string tryme = vtksys::SystemTools::JoinPath(components);
      testing->AddArgument(tryme.c_str());
      continue;
    }

    testing->AddArgument(argv[i]);
  }

  if (testing->IsInteractiveModeSpecified())
  {
    return vtkTesting::DO_INTERACTOR;
  }

  if (testing->IsValidImageSpecified())
  {
    testing->SetRenderWindow(rw);
    return testing->RegressionTest(thresh, cout);
  }

  return vtkTesting::NOT_RUN;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkMultiBaselineRegressionTest.h
