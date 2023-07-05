// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIdList.h"
#include "vtkKdTreePointLocator.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPoissonDiskSampler.h"
#include "vtkSphereSource.h"

#include "vtkXMLPolyDataWriter.h"

//------------------------------------------------------------------------------
int TestPoissonDiskSampler(int, char*[])
{
  double radius = 0.05;

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(200);
  sphere->SetPhiResolution(100);
  sphere->SetRadius(1.0);

  // We run the test 100 times to make failure more likely if there is a bug
  for (int i = 0; i < 100; ++i)
  {
    vtkNew<vtkPoissonDiskSampler> sampler;
    sampler->SetInputConnection(sphere->GetOutputPort());
    sampler->SetRadius(radius);
    sampler->Update();

    vtkPointSet* output = vtkPointSet::SafeDownCast(sampler->GetOutputDataObject(0));

    vtkNew<vtkKdTreePointLocator> locator;
    locator->SetDataSet(output);
    locator->BuildLocator();

    vtkPoints* points = output->GetPoints();
    vtkNew<vtkIdList> ids;

    for (vtkIdType pointId = 0; pointId < output->GetNumberOfPoints(); ++pointId)
    {
      locator->FindPointsWithinRadius(radius, points->GetPoint(pointId), ids);
      if (ids->GetNumberOfIds() > 1)
      {
        vtkLog(ERROR, "Criterion for poisson disk sampling is not met.");
        vtkLog(INFO, << i << "pointIds " << pointId);
        for (vtkIdType id = 0; id < ids->GetNumberOfIds(); ++id)
        {
          vtkLog(INFO, << ids->GetId(id));
        }

        vtkNew<vtkPolyData> outputPD;
        outputPD->ShallowCopy(output);
        vtkNew<vtkXMLPolyDataWriter> writer;
        writer->SetFileName("/home/yohann/Documents/ParaView/build/poisson.vtp");
        writer->SetDataModeToAscii();
        writer->SetInputData(outputPD);
        writer->Write();
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
