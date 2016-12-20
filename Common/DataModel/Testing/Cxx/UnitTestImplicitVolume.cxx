/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestImplicitVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkImplicitVolume.h"
#include "vtkImageData.h"
#include "vtkTestErrorObserver.h"
#include "vtkMathUtilities.h"

#include <cstdio>
#include <sstream>

static vtkSmartPointer<vtkImageData> MakeVolume(int, int, int);

int UnitTestImplicitVolume (int, char*[])
{
  int status = 0;

  const int dim = 5;

  // Create a volume
  vtkSmartPointer<vtkImageData> aVolume =
    MakeVolume(dim, dim, dim);

  // Test empty print
  std::cout << "Testing empty Print...";
  vtkSmartPointer<vtkImplicitVolume> impVol =
    vtkSmartPointer<vtkImplicitVolume>::New();
  std::ostringstream emptyPrint;
  impVol->Print(emptyPrint);
  std::cout << "Passed" << std::endl;

  //. Test error messages
  std::cout << "Testing errors...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  impVol->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  impVol->EvaluateFunction(0.0, 0.0, 0.0);
  int status1 = errorObserver->CheckErrorMessage("Can't evaluate function: either volume is missing or volume has no point data");
  double zero[3], zg[3];;
  zero[0] = zero[1] = zero[2] = 0.0;
  impVol->EvaluateGradient(zero, zg);
  status1 += errorObserver->CheckErrorMessage("Can't evaluate gradient: either volume is missing or volume has no point data");
  if (status1)
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  // Test EvaluateFunction
  std::cout << "Testing EvaluateFunction...";
  int status2 = 0;
  impVol->SetVolume(aVolume);
  impVol->SetOutValue(-1000.0);
  for(int k = 0; k < dim; k++)
  {
    for(int j = 0; j < dim; j++)
    {
      for(int i = 0; i < dim; i++)
      {
        double x = i + .5;
        double y = j + .5;
        double z = k;
        double val = impVol->EvaluateFunction( x + .5, y, z);
        if (x > (dim - 1) || y > (dim - 1) || z > (dim - 1))
        {
          if (val != impVol->GetOutValue())
          {
            std::cout << "For " << x << ", " << y << ", " << z
                      << " expected " << impVol->GetOutValue()
                      << " but got " << val <<std::endl;
            ++status2;
          }
        }
        else if (val != z)
        {
          std::cout << "For " << x << ", " << y << ", " << z << " expected " << z << " but got " << val <<std::endl;
          ++status2;
        }
      }
    }
  }
  if (status2)
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  // Test EvaluateGradient
  std::cout << "Testing EvaluateGradient...";
  int status3 = 0;
  double tol = std::numeric_limits<double>::epsilon();
  double og[3];
  og[0] = og[1] = og[2] = -1000.0;
  impVol->SetOutGradient(og);
  for(int k = 0; k < dim; k++)
  {
    for(int j = 0; j < dim; j++)
    {
      for(int i = 0; i < dim; i++)
      {
        double xyz[3];
        xyz[0] = i + .5;
        xyz[1] = j + .5;
        xyz[2] = k;
        double n[3];
        impVol->EvaluateGradient(xyz, n);
        if (xyz[0] > (dim - 1) || xyz[1] > (dim - 1) || xyz[2] > (dim - 1))
        {
          if (n[0] != impVol->GetOutGradient()[0] ||
              n[1] != impVol->GetOutGradient()[1] ||
              n[2] != impVol->GetOutGradient()[2])
          {
            std::cout << "For " << xyz[0] << ", " << xyz[1] << ", " << xyz[2]
                      << " expected "
                      << impVol->GetOutGradient()[0] << ", "
                      << impVol->GetOutGradient()[1] << ", "
                      << impVol->GetOutGradient()[2]
                      << " but got " << n[0] << ", " << n[1] << ", " << n[2]
                      << std::endl;
            ++status3;
          }
        }
        else if (
          !vtkMathUtilities::FuzzyCompare(0.0, n[0], tol) ||
          !vtkMathUtilities::FuzzyCompare(0.0, n[1], tol) ||
          !vtkMathUtilities::FuzzyCompare(-1.0, n[2], tol))
        {
          std::cout << "For " << xyz[0] << ", " << xyz[1] << ", " << xyz[2]
                    << " expected "
                    << "0, 0, -1"
                    << " but got " << n[0] << ", " << n[1] << ", " << n[2]
                    << std::endl;
          ++status3;
        }
      }
    }
  }
  if (status3)
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  // Test non-empty print
  std::cout << "Testing non-empty Print...";
  std::ostringstream nonemptyPrint;
  impVol->Print(nonemptyPrint);
  std::cout << "Passed" << std::endl;

  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

vtkSmartPointer<vtkImageData> MakeVolume(int dimx, int dimy, int dimz)
{
  vtkSmartPointer<vtkImageData> aVolume =
    vtkSmartPointer<vtkImageData>::New();
  aVolume->SetDimensions(dimx, dimy, dimz);
  aVolume->AllocateScalars(VTK_FLOAT,1);
  float* pixel = static_cast<float *>(aVolume->GetScalarPointer(0,0,0));
  float value = 0.0;

  for(int z = 0; z < dimz; z++)
  {
    for(int y = 0; y < dimy; y++)
    {
      for(int x = 0; x < dimx; x++)
      {
        *pixel++ = value;
      }
    }
    value += 1.0;
  }
  return aVolume;
}
