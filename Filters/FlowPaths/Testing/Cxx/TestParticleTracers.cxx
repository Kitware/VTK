// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGenerateTimeSteps.h"
#include "vtkGradientFilter.h"
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
bool Execute(vtkAlgorithm* input, vtkPolyData* seeds, vtkSmartPointer<vtkDataObject>&& expected)
{
  vtkNew<TracerT> tracer;
  tracer->SetInputConnection(0, input->GetOutputPort());
  tracer->SetInputData(1, seeds);
  tracer->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gradients");

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

int TestParticleTracers(int argc, char* argv[])
{
  auto getBaseline = [&](std::string&& name) {
    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(vtkTestUtilities::ExpandDataFileName(
      argc, argv, (std::string("Data/ParticleTracers/") + name).c_str()));
    reader->Update();
    return vtkSmartPointer<vtkDataObject>(reader->GetOutputDataObject(0));
  };

  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkGradientFilter> gradient;
  vtkNew<vtkGenerateTimeSteps> temporal;
  vtkNew<vtkParticleTracer> tracer;

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

  gradient->SetInputConnection(wavelet->GetOutputPort());
  temporal->SetInputConnection(gradient->GetOutputPort());

  // mimicking catalyst environment
  wavelet->SetNoPriorTemporalAccessInformationKey();

  bool retVal = ::Execute<vtkParticleTracer>(temporal, seeds, getBaseline("tracer.vtp"));
  retVal &= ::Execute<vtkParticlePathFilter>(temporal, seeds, getBaseline("pathline.vtp"));
  retVal &= ::Execute<vtkStreaklineFilter>(temporal, seeds, getBaseline("streakline.vtp"));

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
