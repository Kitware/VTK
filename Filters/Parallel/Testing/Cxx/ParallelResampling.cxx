/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelResampling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests ParallelResampling.

/*
** This test only builds if MPI is in use
*/
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPResampleFilter.h"
#include "vtkPointData.h"
#include "vtkProcess.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTestUtilities.h"
#include "vtkDataSetSurfaceFilter.h"
#include <mpi.h>

namespace
{

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();

  virtual void Execute();

  void SetArgs(int anArgc,
               char *anArgv[]);

protected:
  MyProcess();

  int Argc;
  char **Argv;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc=0;
  this->Argv=0;
}

void MyProcess::SetArgs(int anArgc,
                        char *anArgv[])
{
  this->Argc=anArgc;
  this->Argv=anArgv;
}

void MyProcess::Execute()
{
  this->ReturnValue=1;
  int numProcs=this->Controller->GetNumberOfProcesses();
  int me=this->Controller->GetLocalProcessId();
  cout << "Nb process found: " << numProcs << endl;

  // Create and execute pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkPResampleFilter> sampler;
  vtkNew<vtkDataSetSurfaceFilter> toPolyData;
  vtkNew<vtkPolyDataMapper> mapper;

  sampler->SetInputConnection(wavelet->GetOutputPort());
  sampler->SetSamplingDimension(21,21,21); // 21 for perfect match with wavelet default extent
  //sampler->SetUseInputBounds(0);
  //sampler->SetCustomSamplingBounds(-10, 10, -10, 10, -10, 10);

  toPolyData->SetInputConnection(sampler->GetOutputPort());

  mapper->SetInputConnection(toPolyData->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  cout << "Got for Wavelet " << wavelet->GetOutput()->GetNumberOfPoints() << " points on process " << me << endl;
  cout << "Got for Surface " << toPolyData->GetOutput()->GetNumberOfPoints() << " points on process " << me << endl;

  if (me == 0)
  {
    // Only root node compare the standard Wavelet data with the probed one
    vtkNew<vtkRTAnalyticSource> waveletBase1Piece;
    waveletBase1Piece->Update();
    vtkImageData *reference = waveletBase1Piece->GetOutput();
    vtkImageData *result = sampler->GetOutput();

    // Compare RTData Array
    vtkFloatArray* rtDataRef = vtkArrayDownCast<vtkFloatArray>(reference->GetPointData()->GetArray("RTData"));
    vtkFloatArray* rtDataTest = vtkArrayDownCast<vtkFloatArray>(result->GetPointData()->GetArray("RTData"));
    vtkIdType sizeRef = rtDataRef->GetNumberOfTuples();
    if(sizeRef == rtDataTest->GetNumberOfTuples() && rtDataRef->GetNumberOfComponents() == 1)
    {
      for(vtkIdType idx = 0; idx < sizeRef; ++idx)
      {
        if(rtDataRef->GetValue(idx) != rtDataTest->GetValue(idx))
        {
          this->ReturnValue = 0;
          return;
        }
      }
      return; // OK
    }

    this->ReturnValue = 0;
  }
  else
  {
    if(sampler->GetOutput()->GetNumberOfPoints() != 0 || wavelet->GetOutput()->GetNumberOfPoints() == 0)
    {
      this->ReturnValue = 0;
    }
  }
}

}

int ParallelResampling(int argc, char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1;

  vtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();

  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal;   // is this the right error val?   TODO
  }

  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal=p->GetReturnValue();
  p->Delete();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
