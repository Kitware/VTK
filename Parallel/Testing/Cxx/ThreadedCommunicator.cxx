/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ThreadedCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCharArray.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInputPort.h"
#include "vtkIntArray.h"
#include "vtkOutputPort.h"
#include "vtkParallelFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkThreadedController.h"
#include "vtkUnsignedLongArray.h"

#include "vtkDebugLeaks.h"
#include "vtkRegressionTestImage.h"

static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void TestBarrier(vtkMultiProcessController *contr, void* vtkNotUsed(arg))
{
  contr->Barrier();
}

void Process2(vtkMultiProcessController *contr, void* vtkNotUsed(arg))
{
  int i;

  // Test receiving all supported types of arrays
  int datai[scMsgLength];
  if (!contr->Receive(datai, scMsgLength, 0, 11))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (datai[i] != i)
      {
      cerr << "Server error: Corrupt integer array." << endl;
      }
    }

  unsigned long dataul[scMsgLength];
  if (!contr->Receive(dataul, scMsgLength, 0, 22))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (dataul[i] != static_cast<unsigned long>(i))
      {
      cerr << "Server error: Corrupt unsigned long array." << endl;
      }
    }

  char datac[scMsgLength];
  if (!contr->Receive(datac, scMsgLength, 0, 33))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (datac[i] != static_cast<char>(i))
      {
      cerr << "Server error: Corrupt char array." << endl;
      }
    }

  unsigned char datauc[scMsgLength];
  if (!contr->Receive(datauc, scMsgLength, 0, 44))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (datauc[i] != static_cast<unsigned char>(i))
      {
      cerr << "Server error: Corrupt unsigned char array." << endl;
      }
    }

  float dataf[scMsgLength];
  if (!contr->Receive(dataf, scMsgLength, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (dataf[i] != static_cast<float>(i))
      {
      cerr << "Server error: Corrupt float array." << endl;
      }
    }


  double datad[scMsgLength];
  if (!contr->Receive(datad, scMsgLength, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (datad[i] != static_cast<double>(i))
      {
      cerr << "Server error: Corrupt double array." << endl;
      }
    }

  vtkIdType datait[scMsgLength];
  if (!contr->Receive(datait, scMsgLength, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    return;
    }
  for (i=0; i<scMsgLength; i++)
    {
    if (datait[i] != static_cast<vtkIdType>(i))
      {
      cerr << "Server error: Corrupt vtkIdType array." << endl;
      }
    }

  // Test receiving all supported types of arrays
  vtkIntArray* ia = vtkIntArray::New();
  if (!contr->Receive(ia, 0, 11))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<ia->GetNumberOfTuples(); i++)
    {
    if (ia->GetValue(i) != i)
      {
      cerr << "Server error: Corrupt integer array." << endl;
      }
    }
  ia->Delete();

  // Test the ports and sending a data object
  vtkOutputPort* op = vtkOutputPort::New();
  op->SetController(contr);
  op->SetTag(45);

  // send sphere source
  vtkSphereSource* pd = vtkSphereSource::New();

  op->SetInputConnection(pd->GetOutputPort());
  op->WaitForUpdate();
  pd->Delete();

  op->Delete();
}

void Process1(vtkMultiProcessController *contr, void *arg)
{
  GenericCommunicatorArgs_tmp* args = 
    reinterpret_cast<GenericCommunicatorArgs_tmp*>(arg);

  int i;

  // Test sending all supported types of arrays
  int datai[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datai[i] = i;
    }
  if (!contr->Send(datai, scMsgLength, 1, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  unsigned long dataul[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataul[i] = static_cast<unsigned long>(i);
    }
  if (!contr->Send(dataul, scMsgLength, 1, 22))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  char datac[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datac[i] = static_cast<char>(i);
    }
  if (!contr->Send(datac, scMsgLength, 1, 33))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  unsigned char datauc[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datauc[i] = static_cast<unsigned char>(i);
    }
  if (!contr->Send(datauc, scMsgLength, 1, 44))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  float dataf[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataf[i] = static_cast<float>(i);
    }
  if (!contr->Send(dataf, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }


  double datad[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datad[i] = static_cast<double>(i);
    }
  if (!contr->Send(datad, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  vtkIdType datait[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datait[i] = static_cast<vtkIdType>(i);
    }
  if (!contr->Send(datait, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  // Test sending all supported types of arrays
  vtkIntArray* ia = vtkIntArray::New();
  ia->SetArray(datai, 10, 1);
  if (!contr->Send(ia, 1, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ia->Delete();


  // Test the ports and sending a data object
  vtkInputPort* ip = vtkInputPort::New();
  ip->SetController(contr);
  ip->SetTag(45);
  ip->SetRemoteProcessId(1);

  // Get polydata
  ip->GetPolyDataOutput()->Update();

  vtkPolyDataMapper* pmapper = vtkPolyDataMapper::New();
  pmapper->SetInput(ip->GetPolyDataOutput());

  vtkActor* pactor = vtkActor::New();
  pactor->SetMapper(pmapper);
  pmapper->UnRegister(0);

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(pactor);
  pactor->UnRegister(0);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  ren->UnRegister(0);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Render();

  *(args->retVal) = 
    vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);
  if ( *(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  iren->Delete();
  ip->Delete();
  renWin->Delete();
}

int main(int argc, char** argv)
{
  vtkThreadedController* contr = vtkThreadedController::New();
  contr->Initialize(&argc, &argv);
  contr->CreateOutputWindow();

  vtkParallelFactory* pf = vtkParallelFactory::New();
  vtkObjectFactory::RegisterFactory(pf);
  pf->Delete();

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (contr->IsA("vtkThreadedController"))
    {
    // Set the number of processes to 2 for this example.
    contr->SetNumberOfProcesses(2);
    } 

  // Added for regression test.
  // ----------------------------------------------
  int retVal;
  GenericCommunicatorArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetMultipleMethod(0, Process1, &args);
  contr->SetMultipleMethod(1, Process2, 0);
  contr->MultipleMethodExecute();

  contr->SetSingleMethod(TestBarrier, 0);
  contr->SingleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
