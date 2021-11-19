/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractParticlesOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractParticlesOverTime.h"

#include "vtkArrayCalculator.h"
#include "vtkDelaunay3D.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimeSourceExample.h"

int TestExtractParticlesOverTime(int, char*[])
{
  vtkNew<vtkTimeSourceExample> timeSource;
  timeSource->SetXAmplitude(10);
  timeSource->SetYAmplitude(0);

  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputConnection(timeSource->GetOutputPort());
  calculator->SetAttributeTypeToPointData();
  calculator->AddScalarArrayName("Point Label");
  calculator->SetFunction("\"Point Label\" * 10");
  calculator->SetResultArrayName("point_id");

  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(10, 0, 0);
  vtkNew<vtkDelaunay3D> delaunay;
  delaunay->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkExtractParticlesOverTime> particleExtraction;
  particleExtraction->SetInputConnection(0, calculator->GetOutputPort());
  particleExtraction->SetInputConnection(1, delaunay->GetOutputPort());

  double initialTimeStep = 0.5;
  particleExtraction->UpdateTimeStep(initialTimeStep);

  auto* outputInformation = particleExtraction->GetOutputInformation(0);
  if (!outputInformation)
  {
    vtkLog(ERROR, "no output information");
    return EXIT_FAILURE;
  }

  if (!outputInformation->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    vtkLog(ERROR, "TIME_STEPS key not available in output information");
    return EXIT_FAILURE;
  }

  if (outputInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) != 10)
  {
    vtkLog(ERROR, "wrong number of time steps in output information");
    return EXIT_FAILURE;
  }

  auto* resultDataObject = particleExtraction->GetOutputDataObject(0);
  if (!resultDataObject)
  {
    vtkLog(ERROR, "result is null");
    return EXIT_FAILURE;
  }

  auto* resultDataSet = vtkDataSet::SafeDownCast(resultDataObject);
  if (!resultDataSet)
  {
    vtkLog(ERROR, "result is not a data set");
    return EXIT_FAILURE;
  }

  vtkIdType numberOfPoints = resultDataSet->GetNumberOfPoints();
  if (numberOfPoints != 2)
  {
    vtkLog(ERROR, "wrong number of points");
    return EXIT_FAILURE;
  }

  double expectedFirstCoordinates[3] = { 3.4202, 0, 0 };
  double* firstPointCoordinates = resultDataSet->GetPoint(0);
  for (size_t index = 0; index < 3; ++index)
  {
    if (std::abs(firstPointCoordinates[index] - expectedFirstCoordinates[index]) > 1e-4)
    {
      vtkLog(ERROR,
        "Wrong extracted coordinates, index: " << index
                                               << " expected: " << expectedFirstCoordinates[index]
                                               << " got: " << firstPointCoordinates[index]);
    }
  }

  // Move the sphere and update.
  sphere->SetCenter(0, 0, 0);
  particleExtraction->Update();
  vtkIdType updatedNumberOfPoints =
    vtkDataSet::SafeDownCast(particleExtraction->GetOutputDataObject(0))->GetNumberOfPoints();
  if (updatedNumberOfPoints != 1)
  {
    vtkLog(ERROR, "wrong number of points after source update.");
    return EXIT_FAILURE;
  }

  // Compute with a channel array.
  particleExtraction->SetIdChannelArray("point_id");
  particleExtraction->Update();
  vtkIdType channelNumberOfPoints =
    vtkDataSet::SafeDownCast(particleExtraction->GetOutputDataObject(0))->GetNumberOfPoints();
  if (channelNumberOfPoints != 1)
  {
    vtkLog(ERROR, "wrong number of points with Id Channel Array.");
    return EXIT_FAILURE;
  }

  // Compute with an unknown channel array, should use the Global Ids.
  particleExtraction->SetIdChannelArray("unknown_channel_name");
  particleExtraction->Update();
  vtkIdType unknownChannelNumberOfPoints =
    vtkDataSet::SafeDownCast(particleExtraction->GetOutputDataObject(0))->GetNumberOfPoints();
  if (unknownChannelNumberOfPoints != 1)
  {
    vtkLog(ERROR, "wrong number of points with unknown Id Channel Array.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
