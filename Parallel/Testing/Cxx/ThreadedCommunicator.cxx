#include "vtkThreadedController.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkOutputPort.h"
#include "vtkDebugLeaks.h"
#include "vtkSphereSource.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkInputPort.h"
#include "vtkPolyDataMapper.h"
#include "vtkParallelFactory.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkDebugLeaks.h"
#include "vtkRegressionTestImage.h"

static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void TestBarrier(vtkMultiProcessController *contr, void *arg)
{
  contr->Barrier();
}

void Process1(vtkMultiProcessController *contr, void *arg)
{
  vtkCommunicator* comm = contr->GetCommunicator();

  int i;

  // Test receiving all supported types of arrays
  int datai[scMsgLength];
  if (!comm->Receive(datai, scMsgLength, 1, 11))
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
  if (!comm->Receive(dataul, scMsgLength, 1, 22))
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
  if (!comm->Receive(datac, scMsgLength, 1, 33))
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
  if (!comm->Receive(datauc, scMsgLength, 1, 44))
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
  if (!comm->Receive(dataf, scMsgLength, 1, 7))
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
  if (!comm->Receive(datad, scMsgLength, 1, 7))
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
  if (!comm->Receive(datait, scMsgLength, 1, 7))
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
  if (!comm->Receive(ia, 1, 11))
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

  op->SetInput(pd->GetOutput());
  op->WaitForUpdate();
  pd->Delete();

  op->Delete();
}

void Process2(vtkMultiProcessController *contr, void *arg)
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
  if (!comm->Send(datai, scMsgLength, 0, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  unsigned long dataul[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataul[i] = static_cast<unsigned long>(i);
    }
  if (!comm->Send(dataul, scMsgLength, 0, 22))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  char datac[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datac[i] = static_cast<char>(i);
    }
  if (!comm->Send(datac, scMsgLength, 0, 33))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  unsigned char datauc[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datauc[i] = static_cast<unsigned char>(i);
    }
  if (!comm->Send(datauc, scMsgLength, 0, 44))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  float dataf[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataf[i] = static_cast<float>(i);
    }
  if (!comm->Send(dataf, scMsgLength, 0, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }


  double datad[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datad[i] = static_cast<double>(i);
    }
  if (!comm->Send(datad, scMsgLength, 0, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  vtkIdType datait[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datait[i] = static_cast<vtkIdType>(i);
    }
  if (!comm->Send(datait, scMsgLength, 0, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    return;
    }

  // Test sending all supported types of arrays
  vtkIntArray* ia = vtkIntArray::New();
  ia->SetArray(datai, 10, 1);
  if (!comm->Send(ia, 0, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ia->Delete();


  // Test the ports and sending a data object
  vtkInputPort* ip = vtkInputPort::New();
  ip->SetController(contr);
  ip->SetTag(45);
  ip->SetRemoteProcessId(0);

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

  contr->TriggerRMI(0, vtkMultiProcessController::BREAK_RMI_TAG);

  iren->Delete();
  ip->Delete();
  renWin->Delete();
}

int main(int argc, char** argv)
{
  vtkDebugLeaks::PromptUserOff();

  vtkThreadedController* contr = vtkThreadedController::New();
  contr->Initialize(&argc, &argv);

  vtkParallelFactory* pf = vtkParallelFactory::New();
  vtkObjectFactory::RegisterFactory(pf);
  pf->Delete();


  // This is repeated for the sake of MPI. This one might not
  // get called by the parent process, the first one might not
  // get called by all others.
  vtkDebugLeaks::PromptUserOff();

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

  contr->SetMultipleMethod(0, Process1, 0);
  contr->SetMultipleMethod(1, Process2, &args);
  contr->MultipleMethodExecute();

  contr->SetSingleMethod(TestBarrier, 0);
  contr->SingleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
