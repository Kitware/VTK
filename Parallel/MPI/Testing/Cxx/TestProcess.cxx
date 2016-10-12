/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProcess.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the vtkProcess abstract class.

#include <mpi.h>

#include "vtkProcess.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"

namespace
{

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);

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
  // multiprocess logic
  int numProcs=this->Controller->GetNumberOfProcesses();
  int me=this->Controller->GetLocalProcessId();

  cout << "numProcs=" << numProcs << " me=" << me << endl;
  cout << "executable=" << this->Argv[0] << endl;
  cout << "argc=" << this->Argc << endl;

  const int MY_RETURN_VALUE_MESSAGE=0x11;

  if(me==0)
  {
    // root node
    this->ReturnValue=0;
    int i=1;
    while(i<numProcs)
    {
      this->Controller->Send(&this->ReturnValue,1,i,MY_RETURN_VALUE_MESSAGE);
      ++i;
    }
  }
  else
  {
    // satellites
    this->Controller->Receive(&this->ReturnValue,1,0,MY_RETURN_VALUE_MESSAGE);
  }
}

}

int TestProcess(int argc,char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc,&argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *c=vtkMPIController::New();
  c->Initialize(&argc,&argv,1);

  int retVal = 1;

  vtkMultiProcessController::SetGlobalController(c);

  int numProcs =c->GetNumberOfProcesses();
  int me=c->GetLocalProcessId();

  if(numProcs != 2)
  {
    if (me == 0)
    {
      cout << "DistributedData test requires 2 processes" << endl;
    }
    c->Delete();
    return retVal;
  }

  if (!c->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "TestProcess test requires MPI" << endl;
    }
    c->Delete();
    return retVal;
  }

  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);

  c->SetSingleProcessObject(p);
  c->SingleMethodExecute();

  retVal=p->GetReturnValue();

  p->Delete();
  c->Finalize();
  c->Delete();

  return retVal;
}
