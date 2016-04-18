/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestKernels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkLinearKernel.h"
#include "vtkGaussianKernel.h"
#include "vtkEllipsoidalGaussianKernel.h"
#include "vtkShepardKernel.h"
#include "vtkProbabilisticVoronoiKernel.h"
#include "vtkVoronoiKernel.h"
#include "vtkPointSource.h"
#include "vtkSphereSource.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"

#include <string>
#include <cmath>

template<typename T> int TestProbabilisticKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, std::string description = "");
template<typename T> int TestKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, std::string description = "");

//-----------------------------------------------------------------------------
int UnitTestKernels(int, char*[])
{
  const vtkIdType numberOfPoints = 100000;
  int status = 0;
  {
  vtkSmartPointer<vtkGaussianKernel> kernel =
    vtkSmartPointer<vtkGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkGaussianKernel> (kernel, numberOfPoints, "GaussianKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkGaussianKernel> kernel =
    vtkSmartPointer<vtkGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkGaussianKernel> (kernel, numberOfPoints, "GaussianKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetPowerParameter(10.0);
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(10)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetPowerParameter(1.0);
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(1)");
  }
  {
  vtkSmartPointer<vtkProbabilisticVoronoiKernel> kernel =
    vtkSmartPointer<vtkProbabilisticVoronoiKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkProbabilisticVoronoiKernel> (kernel, numberOfPoints, "ProbabilisticVoronoiKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkProbabilisticVoronoiKernel> kernel =
    vtkSmartPointer<vtkProbabilisticVoronoiKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkProbabilisticVoronoiKernel> (kernel, numberOfPoints, "ProbabilisticVoronoiKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkLinearKernel> kernel =
    vtkSmartPointer<vtkLinearKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkLinearKernel> (kernel, numberOfPoints, "LinearKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkLinearKernel> kernel =
    vtkSmartPointer<vtkLinearKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkLinearKernel> (kernel, numberOfPoints, "LinearKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  kernel->SetSharpness(5.0);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Sharpness(5.0)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  kernel->SetEccentricity(.1);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(.1)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  kernel->SetEccentricity(10.0);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(10.0)");
  }
  {
  vtkSmartPointer<vtkVoronoiKernel> kernel =
    vtkSmartPointer<vtkVoronoiKernel>::New();
  kernel->RequiresInitializationOff();
  status += TestKernel<vtkVoronoiKernel> (kernel, numberOfPoints, "VoronoiKernel");
  }
  return status;
}

template<typename T> int TestProbabilisticKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, std::string description)
{
  int status = 0;
  std::cout << "Testing " << description;
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetPhiResolution(11);
  sphere->SetThetaResolution(21);
  sphere->SetRadius(.5);
  sphere->Update();

  vtkSmartPointer<vtkPointSource> randomSphere =
    vtkSmartPointer<vtkPointSource>::New();
  randomSphere->SetRadius(sphere->GetRadius() * 2.0);;
  randomSphere->SetNumberOfPoints(numberOfPoints);
  randomSphere->Update();
  vtkSmartPointer<vtkDoubleArray> distances =
    vtkSmartPointer<vtkDoubleArray>::New();
  distances->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());

  double refPt[3];
  refPt[0] = 0.0;
  refPt[1] = 0.0;
  refPt[2] = 0.0;
  for (vtkIdType id = 0; id < randomSphere->GetOutput()->GetNumberOfPoints(); ++id)
    {
    double distance;
    double pt[3];
    randomSphere->GetOutput()->GetPoint(id, pt);
    distance = std::sqrt(vtkMath::Distance2BetweenPoints(refPt, pt));
    distances->SetTuple1(id, distance);
    }
  distances->SetName("Distances");

  randomSphere->GetOutput()->GetPointData()->SetScalars(distances);

  vtkSmartPointer<vtkStaticPointLocator> locator =
    vtkSmartPointer<vtkStaticPointLocator>::New();
  locator->SetDataSet(randomSphere->GetOutput());
  double meanProbe = 0.0;
  kernel->Initialize(locator, randomSphere->GetOutput(), randomSphere->GetOutput()->GetPointData());
  for (vtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
    {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    vtkSmartPointer<vtkIdList> ptIds =
      vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkDoubleArray> weights =
      vtkSmartPointer<vtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    vtkSmartPointer<vtkDoubleArray> probabilities =
      vtkSmartPointer<vtkDoubleArray>::New();
    probabilities->SetNumberOfTuples(ptIds->GetNumberOfIds());
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
      {
      double distance;
      double pt[3];
      randomSphere->GetOutput()->GetPoint(p, pt);
      distance = std::sqrt(vtkMath::Distance2BetweenPoints(refPt, pt));
      probabilities->SetTuple1(p, (2.0 - distance) / 2.0);
      }
    kernel->ComputeWeights(point, ptIds, probabilities, weights);

    double scalar;
    randomSphere->GetOutput()->GetPointData()->GetArray("Distances")->GetTuple(id, &scalar);
    double probe = 0.0;
    if (id == 0)
      {
      std::cout << " # points: " << ptIds->GetNumberOfIds();
      }
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
      {
      double value;
      randomSphere->GetOutput()->GetPointData()->GetArray("Distances")->GetTuple(ptIds->GetId(p), &value);;
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
      }
    meanProbe += probe;
    }
  meanProbe /= static_cast<double> (sphere->GetOutput()->GetNumberOfPoints());
  std::cout << " Mean probe:" << meanProbe;

  if (!vtkMathUtilities::FuzzyCompare(meanProbe, .5, .01))
    {
    std::cout << " ERROR: Mean of the probes: " << meanProbe << " is not within .01 of the radius .5";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
    }
  else
    {
    std::cout << " PASSED" << std::endl;
    status = EXIT_SUCCESS;
    }
  return status;
}

template<typename T> int TestKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, std::string description)
{
  int status = 0;
  std::cout << "Testing " << description;
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetPhiResolution(21);
  sphere->SetThetaResolution(21);
  sphere->SetRadius(.5);
  sphere->Update();

  vtkSmartPointer<vtkPointSource> randomSphere =
    vtkSmartPointer<vtkPointSource>::New();
  randomSphere->SetRadius(sphere->GetRadius() * 2.0);
  randomSphere->SetNumberOfPoints(numberOfPoints);
  randomSphere->Update();
  vtkSmartPointer<vtkDoubleArray> distances =
    vtkSmartPointer<vtkDoubleArray>::New();
  distances->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());

  double refPt[3];
  refPt[0] = 0.0;
  refPt[1] = 0.0;
  refPt[2] = 0.0;
  for (vtkIdType id = 0; id < randomSphere->GetOutput()->GetNumberOfPoints(); ++id)
    {
    double distance;
    double pt[3];
    randomSphere->GetOutput()->GetPoint(id, pt);
    distance = std::sqrt(vtkMath::Distance2BetweenPoints(refPt, pt));
    distances->SetTuple1(id, distance);
    }
  distances->SetName("Distances");

  randomSphere->GetOutput()->GetPointData()->SetScalars(distances);

  vtkSmartPointer<vtkStaticPointLocator> locator =
    vtkSmartPointer<vtkStaticPointLocator>::New();
  locator->SetDataSet(randomSphere->GetOutput());
  vtkSmartPointer<vtkIdList> ptIds =
    vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkDoubleArray> weights =
    vtkSmartPointer<vtkDoubleArray>::New();
  double meanProbe = 0.0;
  kernel->Initialize(locator, randomSphere->GetOutput(), randomSphere->GetOutput()->GetPointData());
  for (vtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
    {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    kernel->ComputeBasis(point, ptIds);
    kernel->ComputeWeights(point, ptIds, weights);
    if (id == 0)
      {
      std::cout << " # points: " << ptIds->GetNumberOfIds();
      }
    double scalar;
    randomSphere->GetOutput()->GetPointData()->GetArray("Distances")->GetTuple(id, &scalar);
    double probe = 0.0;
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
      {
      double value;
      randomSphere->GetOutput()->GetPointData()->GetArray("Distances")->GetTuple(ptIds->GetId(p), &value);;
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
      }
    meanProbe += probe;
    }
  meanProbe /= static_cast<double> (sphere->GetOutput()->GetNumberOfPoints());
  std::cout << " Mean probe:" << meanProbe;
  if (!vtkMathUtilities::FuzzyCompare(meanProbe, .5, .01))
    {
    std::cout << "ERROR: Mean of the probes: " << meanProbe << " is not within .01 of the radius .5";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
    }
  else
    {
    std::cout << " PASSED" << std::endl;
    status = EXIT_SUCCESS;
    }
  return status;
}
