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
#include <mpi.h>

#include "vtkActor.h"
#include "vtkCharArray.h"
#include "vtkCallbackCommand.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMPIController.h"
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


static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void Process2(vtkMultiProcessController *contr, void* vtkNotUsed(arg))
{
  vtkCommunicator* comm = contr->GetCommunicator();

  int i, retVal=1;

  // Test receiving all supported types of arrays
  vtkIntArray* ia = vtkIntArray::New();
  if (!comm->Receive(ia, 0, 11))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<ia->GetNumberOfTuples(); i++)
    {
    if (ia->GetValue(i) != i)
      {
      cerr << "Server error: Corrupt integer array." << endl;
      retVal = 0;
      break;
      }
    }
  ia->Delete();

  vtkUnsignedLongArray* ula = vtkUnsignedLongArray::New();
  if (!comm->Receive(ula, 0, 22))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<ula->GetNumberOfTuples(); i++)
    {
    if (ula->GetValue(i) != static_cast<unsigned long>(i))
      {
      cerr << "Server error: Corrupt unsigned long array." << endl;
      retVal = 0;
      break;
      }
    }
  ula->Delete();

  vtkCharArray* ca = vtkCharArray::New();
  if (!comm->Receive(ca, 0, 33))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<ca->GetNumberOfTuples(); i++)
    {
    if (ca->GetValue(i) != static_cast<char>(i))
      {
      cerr << "Server error: Corrupt char array." << endl;
      retVal = 0;
      break;
      }
    }
  ca->Delete();

  vtkUnsignedCharArray* uca = vtkUnsignedCharArray::New();
  if (!comm->Receive(uca, 0, 44))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<uca->GetNumberOfTuples(); i++)
    {
    if (uca->GetValue(i) != static_cast<unsigned char>(i))
      {
      cerr << "Server error: Corrupt unsigned char array." << endl;
      retVal = 0;
      break;
      }
    }
  uca->Delete();

  vtkFloatArray* fa = vtkFloatArray::New();
  if (!comm->Receive(fa, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<fa->GetNumberOfTuples(); i++)
    {
    if (fa->GetValue(i) != static_cast<float>(i))
      {
      cerr << "Server error: Corrupt float array." << endl;
      retVal = 0;
      break;
      }
    }
  fa->Delete();

  vtkDoubleArray* da = vtkDoubleArray::New();
  if (!comm->Receive(da, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<da->GetNumberOfTuples(); i++)
    {
    if (da->GetValue(i) != static_cast<double>(i))
      {
      cerr << "Server error: Corrupt double array." << endl;
      retVal = 0;
      break;
      }
    }
  da->Delete();

  vtkIdTypeArray* ita = vtkIdTypeArray::New();
  if (!comm->Receive(ita, 0, 7))
    {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
    }
  for (i=0; i<ita->GetNumberOfTuples(); i++)
    {
    if (ita->GetValue(i) != static_cast<vtkIdType>(i))
      {
      cerr << "Server error: Corrupt vtkIdType array." << endl;
      retVal = 0;
      break;
      }
    }
  ita->Delete();

  comm->Send(&retVal, 1, 0, 11);
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
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
    *(args->retVal) = 0;
    }
  ita->Delete();

  int remoteRetVal;
  comm->Receive(&remoteRetVal, 1, 1, 11);
  if (!remoteRetVal)
    {
    *(args->retVal) = 0;
    }

}

int main(int argc, char** argv)
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv,1);
  contr->CreateOutputWindow();

  vtkParallelFactory* pf = vtkParallelFactory::New();
  vtkObjectFactory::RegisterFactory(pf);
  pf->Delete();

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
