/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestForceTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkForceTime.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnstructuredGrid.h>

class vtkTimeRTAnalyticSource : public vtkRTAnalyticSource
{
public:
  static vtkTimeRTAnalyticSource *New();
  vtkTypeMacro(vtkTimeRTAnalyticSource, vtkRTAnalyticSource);

protected:
  vtkTimeRTAnalyticSource() {}

  int RequestInformation(vtkInformation * request,
                         vtkInformationVector **inputVector,
                         vtkInformationVector *outputVector) VTK_OVERRIDE
  {
    // get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    double range[2] = {0, 5};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 range, 2);

    double outTimes[6] = {0, 1, 2, 3, 4, 5};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 outTimes, 6);
    vtkRTAnalyticSource::RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  void ExecuteDataWithInformation(vtkDataObject *output, vtkInformation *outInfo) VTK_OVERRIDE
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    vtkImageData *data = vtkImageData::GetData(outInfo);
    int* outExt = data->GetExtent();

    // find the region to loop over
    int maxX = (outExt[1] - outExt[0]) + 1;
    int maxY = (outExt[3] - outExt[2]) + 1;
    int maxZ = (outExt[5] - outExt[4]) + 1;

    vtkNew<vtkDoubleArray> timeArray;
    timeArray->SetName("timeData");
    timeArray->SetNumberOfValues(maxX * maxY * maxZ);
    data->GetPointData()->SetScalars(timeArray.Get());

    double t = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    vtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          timeArray->SetValue(cnt, t + idxX);
        }
      }
    }
  }

private:
  vtkTimeRTAnalyticSource(const vtkTimeRTAnalyticSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTimeRTAnalyticSource&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkTimeRTAnalyticSource);

//------------------------------------------------------------------------------
// Program main
int TestForceTime(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the pipeline to produce the initial grid
  vtkNew<vtkTimeRTAnalyticSource> wavelet;
  vtkNew<vtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(wavelet->GetOutputPort());
  vtkNew<vtkForceTime> forceTime;
  forceTime->SetInputConnection(tetrahedralize->GetOutputPort());
  forceTime->SetForcedTime(1);
  forceTime->IgnorePipelineTimeOn();

  forceTime->UpdateInformation();
  forceTime->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  forceTime->Update();

  if (vtkUnstructuredGrid::SafeDownCast(forceTime->GetOutput(0))
      ->GetPointData()->GetScalars()->GetTuple1(0) != 1)
  {
    std::cerr << "Incorrect data in force time output" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(forceTime->GetOutputPort());
  mapper->SetScalarRange(0, 30);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.Get());

  renderer->AddActor(actor.Get());
  renderer->SetBackground(.3, .6, .3); // Background color green

  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
