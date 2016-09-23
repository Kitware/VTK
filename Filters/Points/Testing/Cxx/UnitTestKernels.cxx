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
#include <sstream>

template<typename T> int TestProbabilisticKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, const std::string &description = "", bool useProbs = true);
template<typename T> int TestKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, const std::string &description = "");

//-----------------------------------------------------------------------------
int UnitTestKernels(int, char*[])
{
  const vtkIdType numberOfPoints = 100000;
  int status = 0;
  {
  vtkSmartPointer<vtkGaussianKernel> kernel =
    vtkSmartPointer<vtkGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  kernel->SetSharpness(5.0);
  kernel->NormalizeWeightsOn();
  status += TestProbabilisticKernel<vtkGaussianKernel> (kernel, numberOfPoints, "GaussianKernel: NClosest(100): Sharpness(5.0)");
  }
  {
  vtkSmartPointer<vtkGaussianKernel> kernel =
    vtkSmartPointer<vtkGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkGaussianKernel> (kernel, numberOfPoints, "GaussianKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetPowerParameter(10.0);
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(10)");
  }
  {
  vtkSmartPointer<vtkShepardKernel> kernel =
    vtkSmartPointer<vtkShepardKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetPowerParameter(1.0);
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkShepardKernel> (kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(1)");
  }
  {
  vtkSmartPointer<vtkProbabilisticVoronoiKernel> kernel =
    vtkSmartPointer<vtkProbabilisticVoronoiKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkProbabilisticVoronoiKernel> (kernel, numberOfPoints, "ProbabilisticVoronoiKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkProbabilisticVoronoiKernel> kernel =
    vtkSmartPointer<vtkProbabilisticVoronoiKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkProbabilisticVoronoiKernel> (kernel, numberOfPoints, "ProbabilisticVoronoiKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkLinearKernel> kernel =
    vtkSmartPointer<vtkLinearKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToNClosest();
  kernel->SetNumberOfPoints(100);
  status += TestProbabilisticKernel<vtkLinearKernel> (kernel, numberOfPoints, "LinearKernel: NClosest(100)");
  }
  {
  vtkSmartPointer<vtkLinearKernel> kernel =
    vtkSmartPointer<vtkLinearKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkLinearKernel> (kernel, numberOfPoints, "LinearKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkLinearKernel> kernel =
    vtkSmartPointer<vtkLinearKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetKernelFootprintToRadius();
  kernel->SetRadius(.05);
  status += TestProbabilisticKernel<vtkLinearKernel> (kernel, numberOfPoints, "LinearKernel: Radius(.05), No Probabilities", false);
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);
  std::ostringstream superPrint;
  kernel->Superclass::Print(superPrint);

  kernel->UseNormalsOff();
  kernel->UseScalarsOn();
  kernel->SetScaleFactor(2.0);

  kernel->SetScalarsArrayName("TestDistances");
  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->UseNormalsOn();
  kernel->SetNormalsArrayName("TestNormals");
  kernel->UseScalarsOff();
  kernel->SetRadius(.05);
  kernel->SetSharpness(5.0);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Sharpness(5.0)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  kernel->SetEccentricity(.1);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(.1)");
  }
  {
  vtkSmartPointer<vtkEllipsoidalGaussianKernel> kernel =
    vtkSmartPointer<vtkEllipsoidalGaussianKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  kernel->SetRadius(.05);
  kernel->SetEccentricity(10.0);
  status += TestKernel<vtkEllipsoidalGaussianKernel> (kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(10.0)");
  }
  {
  vtkSmartPointer<vtkVoronoiKernel> kernel =
    vtkSmartPointer<vtkVoronoiKernel>::New();
  std::ostringstream emptyPrint;
  kernel->Print(emptyPrint);

  kernel->RequiresInitializationOff();
  status += TestKernel<vtkVoronoiKernel> (kernel, numberOfPoints, "VoronoiKernel");
  }
  return status;
}

template<typename T> int TestProbabilisticKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, const std::string &description, bool useProbs)
{
  int status = EXIT_SUCCESS;

  std::cout << "Testing " << description;

  if ( !kernel->IsTypeOf("vtkGeneralizedKernel"))
  {
    std::cout << " ERROR: " << kernel->GetClassName()
              << " is not a subclass of vtkGeneralizedKernel";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }
  if ( !kernel->IsTypeOf("vtkInterpolationKernel"))
  {
    std::cout << " ERROR: " << kernel->GetClassName()
              << " is not a subclass of vtkInterpolationKernel";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }

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

  std::ostringstream fullPrint;
  kernel->Print(fullPrint);

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
    if (useProbs)
    {
      kernel->ComputeWeights(point, ptIds, probabilities, weights);
    }
    else
    {
      kernel->ComputeWeights(point, ptIds, NULL, weights);
    }
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

  // Test for exact points
  vtkSmartPointer<vtkStaticPointLocator> exactLocator =
    vtkSmartPointer<vtkStaticPointLocator>::New();
  exactLocator->SetDataSet(sphere->GetOutput());
  vtkSmartPointer<vtkDoubleArray> radii =
    vtkSmartPointer<vtkDoubleArray>::New();
  radii->SetNumberOfTuples(sphere->GetOutput()->GetNumberOfPoints());
  radii->FillComponent(0, .5);
  sphere->GetOutput()->GetPointData()->SetScalars(radii);
  kernel->Initialize(exactLocator,
                     sphere->GetOutput(),
                     sphere->GetOutput()->GetPointData());
  for (vtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    vtkSmartPointer<vtkIdList> ptIds =
      vtkSmartPointer<vtkIdList>::New();
    kernel->ComputeBasis(point, ptIds);
    vtkSmartPointer<vtkDoubleArray> weights =
      vtkSmartPointer<vtkDoubleArray>::New();
    kernel->ComputeWeights(point, ptIds, NULL, weights);

    double probe = 0.0;
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      sphere->GetOutput()->GetPointData()->GetScalars()->GetTuple(ptIds->GetId(p), &value);;
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    if (!vtkMathUtilities::FuzzyCompare(probe, .5, std::numeric_limits<double>::epsilon()*256.0))
    {
      status = EXIT_FAILURE;
      std::cout << "Expected .5 but got " << probe << std::endl;
    }
  }

  if (status == EXIT_SUCCESS)
  {
    std::cout << " PASSED" << std::endl;
  }
  return status;
}

template<typename T> int TestKernel (vtkSmartPointer<T> kernel, vtkIdType numberOfPoints, const std::string &description)
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
  vtkSmartPointer<vtkDoubleArray> normals =
    vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());

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
    double normal[3];
    normal[0] = pt[0];
    normal[1] = pt[1];
    normal[2] = pt[2];
    normals->SetTuple3(id, normal[0], normal[1], normal[2]);
  }
  distances->SetName("TestDistances");
  normals->SetName("TestNormals");

  randomSphere->GetOutput()->GetPointData()->AddArray(distances);
  randomSphere->GetOutput()->GetPointData()->AddArray(normals);

  vtkSmartPointer<vtkStaticPointLocator> locator =
    vtkSmartPointer<vtkStaticPointLocator>::New();
  locator->SetDataSet(randomSphere->GetOutput());
  double meanProbe = 0.0;
  kernel->Initialize(locator, randomSphere->GetOutput(), randomSphere->GetOutput()->GetPointData());

  std::ostringstream fullPrint;
  kernel->Print(fullPrint);
  for (vtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    vtkSmartPointer<vtkIdList> ptIds =
      vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkDoubleArray> weights =
      vtkSmartPointer<vtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    kernel->ComputeWeights(point, ptIds, weights);
    if (id == 0)
    {
      std::cout << " # points: " << ptIds->GetNumberOfIds();
    }
    double scalar;
    randomSphere->GetOutput()->GetPointData()->GetArray("TestDistances")->GetTuple(id, &scalar);
    double probe = 0.0;
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      randomSphere->GetOutput()->GetPointData()->GetArray("TestDistances")->GetTuple(ptIds->GetId(p), &value);;
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

  // Test for exact points
  vtkSmartPointer<vtkStaticPointLocator> exactLocator =
    vtkSmartPointer<vtkStaticPointLocator>::New();
  exactLocator->SetDataSet(sphere->GetOutput());
  vtkSmartPointer<vtkDoubleArray> radii =
    vtkSmartPointer<vtkDoubleArray>::New();
  radii->SetNumberOfTuples(sphere->GetOutput()->GetNumberOfPoints());
  radii->FillComponent(0, .5);
  sphere->GetOutput()->GetPointData()->SetScalars(radii);
  kernel->Initialize(exactLocator,
                     sphere->GetOutput(),
                     sphere->GetOutput()->GetPointData());
  for (vtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    vtkSmartPointer<vtkIdList> ptIds =
      vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkDoubleArray> weights =
      vtkSmartPointer<vtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    kernel->ComputeWeights(point, ptIds, weights);

    double probe = 0.0;
    for (vtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      sphere->GetOutput()->GetPointData()->GetScalars()->GetTuple(ptIds->GetId(p), &value);;
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    if (!vtkMathUtilities::FuzzyCompare(probe, .5, std::numeric_limits<double>::epsilon()*256.0))
    {
      status = EXIT_FAILURE;
      std::cout << "Expected .5 but got " << probe << std::endl;
    }
  }

  if (status == EXIT_SUCCESS)
  {
    std::cout << " PASSED" << std::endl;
  }
  return status;
}
