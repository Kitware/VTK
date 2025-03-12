// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendDataSets.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGradientFilter.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkParticlePathFilter.h"
#include "vtkParticleTracer.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkStreaklineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

#include <numeric>
#include <vector>

namespace
{
template <class TracerT>
bool Execute(vtkAlgorithm* input, vtkPolyData* seeds, bool vorticity,
  vtkSmartPointer<vtkDataObject>&& expected)
{
  vtkNew<TracerT> tracer;
  tracer->SetInputConnection(0, input->GetOutputPort());
  tracer->SetInputData(1, seeds);
  tracer->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gradients");
  tracer->SetComputeVorticity(vorticity);

  for (int t = 0; t < 10; ++t)
  {
    tracer->UpdateTimeStep(t);
  }

  if (!vtkTestUtilities::CompareDataObjects(tracer->GetOutputDataObject(0), expected))
  {
    vtkLog(ERROR, "Tracer of type " << tracer->GetClassName() << " failed.");
    return false;
  }

  return true;
}
} // anonymous namespace

bool TestParticleTracersInput(
  int argc, char* argv[], vtkDataSet* input, std::string_view prefix, bool vorticity)
{
  auto getBaseline = [&](std::string&& name)
  {
    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(vtkTestUtilities::ExpandDataFileName(
      argc, argv, (std::string("Data/ParticleTracers/") + name).c_str()));
    reader->Update();
    return vtkSmartPointer<vtkDataObject>(reader->GetOutputDataObject(0));
  };

  vtkNew<vtkGradientFilter> gradient;
  vtkNew<vtkGenerateTimeSteps> temporal;

  vtkNew<vtkPolyData> seeds;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(3);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 1, 1);
  points->SetPoint(2, -1, -1, -1);
  seeds->SetPoints(points);

  std::vector<double> timesteps(10);
  std::iota(timesteps.begin(), timesteps.end(), 0);
  temporal->SetTimeStepValues(10, timesteps.data());

  gradient->SetInputData(input);
  temporal->SetInputConnection(gradient->GetOutputPort());

  bool retVal = true;
  ::Execute<vtkParticleTracer>(
    temporal, seeds, vorticity, getBaseline(std::string(prefix) + "tracer.vtp"));
  retVal &= ::Execute<vtkParticlePathFilter>(
    temporal, seeds, vorticity, getBaseline(std::string(prefix) + "pathline.vtp"));
  retVal &= ::Execute<vtkStreaklineFilter>(
    temporal, seeds, vorticity, getBaseline(std::string(prefix) + "streakline.vtp"));

  if (!retVal)
  {
    vtkLog(ERROR, "With an input of type " << input->GetClassName());
  }
  return retVal;
}

int TestParticleTracers(int argc, char* argv[])
{
  bool retVal = true;

  // Test image input
  vtkNew<vtkRTAnalyticSource> wavelet;
  // mimicking catalyst environment
  wavelet->SetNoPriorTemporalAccessInformationKey();
  wavelet->Update();
  retVal &= ::TestParticleTracersInput(argc, argv, wavelet->GetOutput(), "image_", true);

  // Test unstructured grid input
  vtkNew<vtkAppendDataSets> append;
  append->SetInputConnection(wavelet->GetOutputPort());
  append->Update();

  // Because of https://gitlab.kitware.com/vtk/vtk/-/issues/19632, disable vorticity computation
  retVal &= ::TestParticleTracersInput(argc, argv, append->GetOutput(), "ug_", false);

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
