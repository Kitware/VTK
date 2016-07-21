/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRegressionTestImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkRegressionTestImage_h
#define vtkRegressionTestImage_h

// Includes and a macro necessary for saving the image produced by a cxx
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "vtkTesting.h"

class vtkRegressionTester : public vtkTesting
{
protected:
  vtkRegressionTester() {}
  ~vtkRegressionTester() {}
private:
  vtkRegressionTester(const vtkRegressionTester&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRegressionTester&) VTK_DELETE_FUNCTION;
};

#define vtkRegressionTestImage(rw) \
vtkTesting::Test(argc, argv, rw, 10)

#define vtkRegressionTestImageThreshold(rw, t) \
vtkTesting::Test(argc, argv, rw, t)

#endif // vtkRegressionTestImage_h
// VTK-HeaderTest-Exclude: vtkRegressionTestImage.h
