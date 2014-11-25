/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestADIOSSphereWR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>

#include <vtkNew.h>
#include <vtkAlgorithm.h>
#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkMPIController.h>
#include <vtkMultiProcessController.h>

#include <vtkSphereSource.h>
#include <vtkADIOSWriter.h>

#include <vtkADIOSReader.h>

class ValidateSphere : public vtkAlgorithm
{
public:
  static ValidateSphere* New();
  vtkTypeMacro(ValidateSphere,vtkAlgorithm);

  ValidateSphere()
  : Valid(true)
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(0);
  }

  virtual int FillInputPortInformation(int vtkNotUsed(port),
    vtkInformation* info)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    return 1;
  }

  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

  bool IsValid() const { return this->Valid; }

private:
  bool Valid;
  std::vector<double> TimeSteps;
  int CurrentTimeStepIndex;
};
vtkStandardNewMacro(ValidateSphere);

//----------------------------------------------------------------------------
int ValidateSphere::ProcessRequest(vtkInformation* request,
  vtkInformationVector** input, vtkInformationVector* output)
{
  vtkMultiProcessController *controller =
    vtkMultiProcessController::GetGlobalController();

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkInformation *inInfo = input[0]->GetInformationObject(0);
    if(!inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
      {
      vtkErrorMacro("No time steps are present");
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }
    int numSteps = inInfo->Length(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if(numSteps != 10)
      {
      vtkErrorMacro("Unexpected number of steps");
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }
    double *steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeSteps.clear();
    this->TimeSteps.reserve(numSteps);
    this->TimeSteps.insert(this->TimeSteps.begin(), steps, steps+numSteps);
    this->CurrentTimeStepIndex = 0;
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation *inInfo = input[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      controller->GetNumberOfProcesses());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      controller->GetLocalProcessId());

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      this->TimeSteps[this->CurrentTimeStepIndex]);

    return 1;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    vtkDataObject *input = this->GetInputDataObject(0, 0);
    vtkMultiBlockDataSet *mbInput = vtkMultiBlockDataSet::SafeDownCast(input);

    if(mbInput->GetNumberOfBlocks() != 1)
      {
      vtkErrorMacro("Incorrect number of blocks");
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }

    vtkMultiPieceDataSet *mpInput =
      vtkMultiPieceDataSet::SafeDownCast(mbInput->GetBlock(0));
    if(mpInput->GetNumberOfPieces() != controller->GetNumberOfProcesses())
      {
      vtkErrorMacro("Number of pieces read != number of pieces written");
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }

    int theta = 10;
    int phi = 10+this->CurrentTimeStepIndex - 2;

    vtkPolyData *pdInput = vtkPolyData::SafeDownCast(
      mpInput->GetPiece(controller->GetLocalProcessId()));

    int numCellsLocal = pdInput->GetNumberOfCells();
    int numCellsGlobal;
    controller->AllReduce(&numCellsLocal, &numCellsGlobal, 1,
      vtkCommunicator::SUM_OP);
    if(numCellsGlobal != theta*phi*2)
      {
      vtkErrorMacro("Number of cells " << numCellsLocal << ","
                    << numCellsGlobal << " != " << theta*phi*2);
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }

    vtkPoints *points = pdInput->GetPoints();
    int numPointsLocal = points->GetNumberOfPoints();
    int numPointsGlobal;
    controller->AllReduce(&numPointsLocal, &numPointsGlobal, 1,
      vtkCommunicator::SUM_OP);
    if(numPointsGlobal != theta*phi+2)
      {
      vtkErrorMacro("Number of points " << numPointsLocal << ","
                    << numPointsGlobal << " != " << theta*phi+2);
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Valid = false;
      return 0;
      }

    // Advance to the next time step
    ++this->CurrentTimeStepIndex;
    if(CurrentTimeStepIndex >= this->TimeSteps.size())
      {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      }
    else
      {
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
    return 1;
    }

  return this->Superclass::ProcessRequest(request, input, output);
}

//----------------------------------------------------------------------------

int TestADIOSSphereWR(int argc, char *argv[])
{
  bool Success = true;
    {
    vtkNew<vtkMPIController> controller;
    controller->Initialize(&argc, &argv, 0);
    vtkMultiProcessController::SetGlobalController(controller.GetPointer());


      // Write out a sphere who's radius changes over time
      std::cout << "Begin vtkADIOSWriter test" << std::endl;
      {
      vtkNew<vtkSphereSource> sphere;
      vtkNew<vtkADIOSWriter> writer;

      writer->SetInputConnection(sphere->GetOutputPort());

      writer->SetFileName("sphere.bp");
      writer->SetWriteAllTimeSteps(1);
      writer->SetTransportMethodToMPI();

      sphere->SetThetaResolution(10);
      for(int t = 0; t < 10; ++t)
        {
        double r = 10+t;
        std::cout << "Setting sphere phi resolution " << r << std::endl;
        sphere->SetPhiResolution(r);

        std::cout << "Writing time step" << std::endl;
        writer->Update();
        }
      }
      std::cout << "End vtkADIOSWriter test" << std::endl;

      // Read back in the expected number of pieces
      std::cout << "Begin vtkADIOSReader test" << std::endl;
      {
      vtkNew<vtkADIOSReader> reader;
      vtkNew<ValidateSphere> validate;

      validate->SetInputConnection(reader->GetOutputPort());

      reader->SetFileName("sphere.bp");
      validate->UpdateInformation();
      validate->Update();
      Success = validate->IsValid();
      }

    vtkMultiProcessController::SetGlobalController(NULL);
    controller->Finalize();
    }
  return Success ? 0 : 1;
}
