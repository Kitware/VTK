/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestComputeBoundingSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkSphere.h>
#include <vtkMath.h>

#include <vector>
#include <cstdlib>

int TestComputeBoundingSphere(int, char*[])
{
  int status = 0;

  const size_t numberOfPoints = 1000;
  const size_t numberOfSpheres = 100;

  {
  std::cout << "Testing 0 points...";
  float *points = NULL;
  float sphere[4];
  vtkSphere::ComputeBoundingSphere(points,
                                   0,
                                   sphere,
                                   NULL);
  if (sphere[0] == 0.0 &&
      sphere[1] == 0.0 &&
      sphere[2] == 0.0 &&
      sphere[3] == 0.0)
  {
    std::cout << "Passed" << std::endl;
  }
  else
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  }

  {
  std::cout << "Testing 1 point...";
  std::vector<double> doublePoints;
  for (size_t i = 0; i < 1; ++i)
  {
    double x =vtkMath::Random(-100.0, 100.0);
    double y =vtkMath::Random(-10.0, 10.0);
    double z =vtkMath::Random(-1.0, 1.0);
    doublePoints.push_back(x);
    doublePoints.push_back(y);
    doublePoints.push_back(z);
  }
  double sphere[4];
  vtkSphere::ComputeBoundingSphere(&(*doublePoints.begin()),
                                   1,
                                   sphere,
                                   NULL);
  if (sphere[0] == doublePoints[0] &&
      sphere[1] == doublePoints[1] &&
      sphere[2] == doublePoints[2] &&
      sphere[3] == 0.0)
  {
    std::cout << "Passed" << std::endl;
  }
  else
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  }

  {
  std::cout << "Testing ComputeBoundingSphere(double) " << numberOfPoints << " points...";
  std::vector<double> doublePoints;
  for (size_t i = 0; i < numberOfPoints; ++i)
  {
    double x =vtkMath::Random(-100.0, 100.0);
    double y =vtkMath::Random(-10.0, 10.0);
    double z =vtkMath::Random(-1.0, 1.0);
    doublePoints.push_back(x);
    doublePoints.push_back(y);
    doublePoints.push_back(z);
  }
  double sphere[4];
  vtkSphere::ComputeBoundingSphere(&(*doublePoints.begin()),
                                   numberOfPoints,
                                   sphere,
                                   NULL);
  std::cout << "sphere: "
            << sphere[0] << ", "
            << sphere[1] << ", "
            << sphere[2] << ": "
            << sphere[3] << " ";
  std::cout << "Passed" << std::endl;
  }

  {
  std::cout << "Testing ComputeBoundingSphere(float) " << numberOfPoints << " points...";
  std::vector<float> floatPoints;
  floatPoints.push_back(-100.0);
  floatPoints.push_back(0.0);
  floatPoints.push_back(0.0);
  floatPoints.push_back(100.0);
  floatPoints.push_back(0.0);
  floatPoints.push_back(0.0);
  for (size_t i = 2; i < numberOfPoints; ++i)
  {
    float x =vtkMath::Random(-100.0, 100.0);
    float y =vtkMath::Random(-100.0, 100.0);
    float z =vtkMath::Random(-100.0, 100.0);
    floatPoints.push_back(x);
    floatPoints.push_back(y);
    floatPoints.push_back(z);
  }
  vtkIdType hint[2];
  hint[0] = 0;
  hint[1] = 1;

  float sphere[4];
  vtkSphere::ComputeBoundingSphere(&(*floatPoints.begin()),
                                   numberOfPoints,
                                   sphere,
                                   hint);
  std::cout << "sphere: "
            << sphere[0] << ", "
            << sphere[1] << ", "
            << sphere[2] << ": "
            << sphere[3] << " ";
  std::cout << "Passed" << std::endl;
  }

  {
  std::cout << "Testing ComputeBoundingSphere(double) " << numberOfPoints << " points...";
  std::vector<double> doublePoints;
  for (size_t i = 0; i < numberOfPoints; ++i)
  {
    double x =vtkMath::Random(-100.0, 100.0);
    double y =vtkMath::Random(-10.0, 10.0);
    double z =vtkMath::Random(-1.0, 1.0);
    doublePoints.push_back(x);
    doublePoints.push_back(y);
    doublePoints.push_back(z);
  }
  double sphere[4];
  vtkSphere::ComputeBoundingSphere(&(*doublePoints.begin()),
                                   numberOfPoints,
                                   sphere,
                                   NULL);
  std::cout << "sphere: "
            << sphere[0] << ", "
            << sphere[1] << ", "
            << sphere[2] << ": "
            << sphere[3] << " ";
  std::cout << "Passed" << std::endl;
  }

  {
  std::cout << "Testing 0 spheres...";
  float **spheres = NULL;
  float sphere[4];
  vtkSphere::ComputeBoundingSphere(spheres,
                                   0,
                                   sphere,
                                   NULL);
  if (sphere[0] == 0.0 &&
      sphere[1] == 0.0 &&
      sphere[2] == 0.0 &&
      sphere[3] == 0.0)
  {
    std::cout << "Passed" << std::endl;
  }
  else
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  }

  {
  std::cout << "Testing 1 sphere...";
  std::vector<std::vector<float> > floatPoints;
  for (size_t i = 0; i < 1; ++i)
  {
    std::vector<float> xyzr(4);
    xyzr[0] = vtkMath::Random(-100.0, 100.0);
    xyzr[1] = vtkMath::Random(-100.0, 100.0);
    xyzr[2] = vtkMath::Random(-100.0, 100.0);
    xyzr[3] = vtkMath::Random(1.0, 2.0);
    floatPoints.push_back(xyzr);
  }
  std::vector<float *> floatSpheres (floatPoints.size());
  for (size_t i = 0; i < floatSpheres.size(); ++i)
  {
    floatSpheres[i] = &(floatPoints[i][0]);
  }
  float sphere[4];
  vtkSphere::ComputeBoundingSphere(&floatSpheres[0],
                                   1,
                                   sphere,
                                   NULL);
  if (sphere[0] == floatPoints[0][0] &&
      sphere[1] == floatPoints[0][1] &&
      sphere[2] == floatPoints[0][2] &&
      sphere[3] == floatPoints[0][3])
  {
    std::cout << "Passed" << std::endl;
  }
  else
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  }

  {
  std::cout << "Testing ComputeBoundingSphere(float) " << numberOfSpheres << " spheres...";
  std::vector<std::vector<float> > floatPoints;
  for (size_t i = 0; i < numberOfSpheres; ++i)
  {
    std::vector<float> xyzr(4);
    xyzr[0] = vtkMath::Random(-100.0, 100.0);
    xyzr[1] = vtkMath::Random(-100.0, 100.0);
    xyzr[2] = vtkMath::Random(-100.0, 100.0);
    xyzr[3] = vtkMath::Random(1.0, 2.0);
    floatPoints.push_back(xyzr);
  }
  std::vector<float *> floatSpheres (floatPoints.size());
  for (size_t i = 0; i < floatSpheres.size(); ++i)
  {
    floatSpheres[i] = &(floatPoints[i][0]);
  }
  vtkIdType hint[2];
  hint[0] = 0;
  hint[1] = 1;

  float sphere[4];
  vtkSphere::ComputeBoundingSphere(&floatSpheres[0],
                                   numberOfSpheres,
                                   sphere,
                                   hint);
  std::cout << "sphere: "
            << sphere[0] << ", "
            << sphere[1] << ", "
            << sphere[2] << ": "
            << sphere[3] << " ";
  std::cout << "Passed" << std::endl;
  }

  {
  std::cout << "Testing ComputeBoundingSphere(double) " << numberOfSpheres << " spheres...";
  std::vector<std::vector<double> > doublePoints;
  std::vector<double> xyzr(4);
  xyzr[0] = -100.0;
  xyzr[1] = 0.0;
  xyzr[2] = 0.0;
  xyzr[3] = 1.0;
  doublePoints.push_back(xyzr);

  xyzr[0] = 100.0;
  xyzr[1] = 0.0;
  xyzr[2] = 0.0;
  xyzr[3] = 1.0;
  doublePoints.push_back(xyzr);

  for (size_t i = 2; i < numberOfSpheres; ++i)
  {
    xyzr[0] = vtkMath::Random(-100.0, 100.0);
    xyzr[1] = vtkMath::Random(-100.0, 100.0);
    xyzr[2] = vtkMath::Random(-100.0, 100.0);
    xyzr[3] = vtkMath::Random(1.0, 2.0);
    doublePoints.push_back(xyzr);
  }
  std::vector<double *> doubleSpheres (doublePoints.size());
  for (size_t i = 0; i < doubleSpheres.size(); ++i)
  {
    doubleSpheres[i] = &(doublePoints[i][0]);
  }
  double sphere[4];
  vtkSphere::ComputeBoundingSphere(&doubleSpheres[0],
                                   numberOfSpheres,
                                   sphere,
                                   NULL);
  std::cout << "sphere: "
            << sphere[0] << ", "
            << sphere[1] << ", "
            << sphere[2] << ": "
            << sphere[3] << " ";
  std::cout << "Passed" << std::endl;
  }

  return EXIT_SUCCESS;
}
