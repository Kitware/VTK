#include "vtkMultiProcessController.h"
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

#include "vtkDebugLeaks.h"
#include "vtkRegressionTestImage.h"

static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void Process1(vtkMultiProcessController *contr, void *arg)
{
  vtkCommunicator* comm = contr->GetCommunicator();

  int i;

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

  vtkUnsignedLongArray* ula = vtkUnsignedLongArray::New();
  if (!comm->Receive(ula, 1, 22))
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
  if (!comm->Receive(ca, 1, 33))
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
  if (!comm->Receive(uca, 1, 44))
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
  if (!comm->Receive(fa, 1, 7))
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
  if (!comm->Receive(da, 1, 7))
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
  if (!comm->Receive(ita, 1, 7))
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
  vtkIntArray* ia = vtkIntArray::New();
  ia->SetArray(datai, 10, 1);
  if (!comm->Send(ia, 0, 11))
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
  if (!comm->Send(ula, 0, 22))
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
  if (!comm->Send(ca, 0, 33))
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
  if (!comm->Send(uca, 0, 44))
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
  if (!comm->Send(fa, 0, 7))
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
  if (!comm->Send(da, 0, 7))
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
  if (!comm->Send(ita, 0, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    }
  ita->Delete();

  // Test the ports
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

  renWin->Render();

  *(args->retVal) = 
    vtkRegressionTestImage2(args->argc, args->argv, renWin, 10);

  contr->TriggerRMI(0, vtkMultiProcessController::BREAK_RMI_TAG);

  ip->Delete();
  renWin->Delete();
}

int main(int argc, char** argv)
{
  vtkDebugLeaks::PromptUserOff();

  vtkMultiProcessController* contr = vtkMultiProcessController::New();
  contr->Initialize(&argc, &argv);

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
  
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
