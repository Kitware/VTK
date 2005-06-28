/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GenericCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCharArray.h"
#include "vtkCallbackCommand.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInputPort.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkOutputPort.h"
#include "vtkParallelFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedLongArray.h"
#include "vtkImageData.h"

#include "vtkDebugLeaks.h"
#include "vtkRegressionTestImage.h"

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

static void UpdateXFreq(vtkObject *vtkNotUsed( caller ),
                        unsigned long vtkNotUsed(eventId), 
                        void *cd, void *)
{
  vtkRTAnalyticSource* id = reinterpret_cast<vtkRTAnalyticSource*>(cd);
  id->SetXFreq(id->GetXFreq()+20);
}

void Process2(vtkMultiProcessController *contr, void* vtkNotUsed(arg))
{
  vtkCommunicator* comm = contr->GetCommunicator();

  int i;

  // Test receiving all supported types of arrays
  vtkIntArray* ia = vtkIntArray::New();
  if (!comm->Receive(ia, 0, 11))
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

  vtkUnsignedLongArray* ula = vtkUnsignedLongArray::New();
  if (!comm->Receive(ula, 0, 22))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<ula->GetNumberOfTuples(); i++)
    {
    if (ula->GetValue(i) != static_cast<unsigned long>(i))
      {
      cerr << "Server error: Corrupt unsigned long array." << endl;
      }
    }
  ula->Delete();

  vtkCharArray* ca = vtkCharArray::New();
  if (!comm->Receive(ca, 0, 33))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<ca->GetNumberOfTuples(); i++)
    {
    if (ca->GetValue(i) != static_cast<char>(i))
      {
      cerr << "Server error: Corrupt char array." << endl;
      }
    }
  ca->Delete();

  vtkUnsignedCharArray* uca = vtkUnsignedCharArray::New();
  if (!comm->Receive(uca, 0, 44))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<uca->GetNumberOfTuples(); i++)
    {
    if (uca->GetValue(i) != static_cast<unsigned char>(i))
      {
      cerr << "Server error: Corrupt unsigned char array." << endl;
      }
    }
  uca->Delete();

  vtkFloatArray* fa = vtkFloatArray::New();
  if (!comm->Receive(fa, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<fa->GetNumberOfTuples(); i++)
    {
    if (fa->GetValue(i) != static_cast<float>(i))
      {
      cerr << "Server error: Corrupt float array." << endl;
      }
    }
  fa->Delete();

  vtkDoubleArray* da = vtkDoubleArray::New();
  if (!comm->Receive(da, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<da->GetNumberOfTuples(); i++)
    {
    if (da->GetValue(i) != static_cast<double>(i))
      {
      cerr << "Server error: Corrupt double array." << endl;
      }
    }
  da->Delete();

  vtkIdTypeArray* ita = vtkIdTypeArray::New();
  if (!comm->Receive(ita, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    }
  for (i=0; i<ita->GetNumberOfTuples(); i++)
    {
    if (ita->GetValue(i) != static_cast<vtkIdType>(i))
      {
      cerr << "Server error: Corrupt vtkIdType array." << endl;
      }
    }
  ita->Delete();

  vtkOutputPort* op = vtkOutputPort::New();
  op->SetController(contr);
  op->SetTag(45);

  int extent = 20;
  vtkRTAnalyticSource* id = vtkRTAnalyticSource::New();
  id->SetWholeExtent (0, 2*extent, 0, 2*extent, 0, 2*extent); 
  id->SetCenter(extent, extent, extent);
  id->SetStandardDeviation( 0.5 );
  id->SetMaximum( 255.0 );
  id->SetXFreq( 60 );
  id->SetXMag( 10 );
  id->SetYFreq( 30 );
  id->SetYMag( 18 );
  id->SetZFreq( 40 );
  id->SetZMag( 5 );
  id->GetOutput()->SetSpacing(2.0/extent,2.0/extent,2.0/extent);

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(UpdateXFreq);
  cbc->SetClientData((void *)id);
  op->AddObserver(vtkCommand::EndEvent,cbc);
  cbc->Delete();
                  
  op->SetInput(id->GetOutput());
  op->WaitForUpdate();
  id->Delete();

  op->Delete();
}

void Process1(vtkMultiProcessController *contr, void *arg)
{
  GenericCommunicatorArgs_tmp* args = 
    reinterpret_cast<GenericCommunicatorArgs_tmp*>(arg);

  vtkCommunicator* comm = contr->GetCommunicator();

  int i;

  // Test sending all supported types of arrays
  int datai[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datai[i] = i;
    }
  vtkIntArray* ia = vtkIntArray::New();
  ia->SetArray(datai, 10, 1);
  if (!comm->Send(ia, 1, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ia->Delete();

  unsigned long dataul[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataul[i] = static_cast<unsigned long>(i);
    }
  vtkUnsignedLongArray* ula = vtkUnsignedLongArray::New();
  ula->SetArray(dataul, 10, 1);
  if (!comm->Send(ula, 1, 22))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ula->Delete();

  char datac[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datac[i] = static_cast<char>(i);
    }
  vtkCharArray* ca = vtkCharArray::New();
  ca->SetArray(datac, 10, 1);
  if (!comm->Send(ca, 1, 33))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ca->Delete();

  unsigned char datauc[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datauc[i] = static_cast<unsigned char>(i);
    }
  vtkUnsignedCharArray* uca = vtkUnsignedCharArray::New();
  uca->SetArray(datauc, 10, 1);
  if (!comm->Send(uca, 1, 44))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  uca->Delete();

  float dataf[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataf[i] = static_cast<float>(i);
    }
  vtkFloatArray* fa = vtkFloatArray::New();
  fa->SetArray(dataf, 10, 1);
  if (!comm->Send(fa, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  fa->Delete();


  double datad[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datad[i] = static_cast<double>(i);
    }
  vtkDoubleArray* da = vtkDoubleArray::New();
  da->SetArray(datad, 10, 1);
  if (!comm->Send(da, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  da->Delete();

  vtkIdType datait[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datait[i] = static_cast<vtkIdType>(i);
    }
  vtkIdTypeArray* ita = vtkIdTypeArray::New();
  ita->SetArray(datait, 10, 1);
  if (!comm->Send(ita, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ita->Delete();

  // Test the ports
  vtkInputPort* ip = vtkInputPort::New();
  ip->SetController(contr);
  ip->SetTag(45);
  ip->SetRemoteProcessId(1);

  // Get polydata
  ip->GetImageDataOutput()->Update();

  vtkContourFilter* cf = vtkContourFilter::New();
  cf->SetInput(ip->GetImageDataOutput());
  
  cf->SetNumberOfContours(1);
  cf->SetValue(0, 220);

  cf->Update();
  cf->Update();
  cf->Update();
  cf->Update();
  cf->Update();
  cf->Update();
  cf->Update();

  vtkPolyData* pd = vtkPolyData::New();
  pd->ShallowCopy(cf->GetOutput());
  cf->Delete();

  vtkPolyDataMapper* pmapper = vtkPolyDataMapper::New();
  pmapper->SetInput(pd);
  pd->Delete();

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
  iren->Initialize();

  renWin->Render();

  *(args->retVal) = 
    vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);

  if ( *(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();

  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  ip->Delete();
  renWin->Delete();
}

int main(int argc, char** argv)
{
#ifndef VTK_LEGACY_REMOVE
  vtkDebugLeaks::PromptUserOff();
#endif

#ifdef VTK_USE_MPI
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
#endif

  vtkMultiProcessController* contr = vtkMultiProcessController::New();
  contr->Initialize(&argc, &argv,1);
  contr->CreateOutputWindow();

  vtkParallelFactory* pf = vtkParallelFactory::New();
  vtkObjectFactory::RegisterFactory(pf);
  pf->Delete();

  // This is repeated for the sake of MPI. This one might not
  // get called by the parent process, the first one might not
  // get called by all others.
#ifndef VTK_LEGACY_REMOVE
  vtkDebugLeaks::PromptUserOff();
#endif

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
  int retVal = 1;
  GenericCommunicatorArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetMultipleMethod(0, Process1, &args);
  contr->SetMultipleMethod(1, Process2, 0);
  contr->MultipleMethodExecute();
  
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
