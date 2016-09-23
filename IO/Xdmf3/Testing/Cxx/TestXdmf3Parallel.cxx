/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXdmf3Parallel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises xdmf3 reading in parallel.
//

#include <mpi.h>

#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkProcess.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXdmf3Reader.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute();

  void SetArgs(int argc, char *argv[], std::string fname)
  {
      this->Argc = argc;
      this->Argv = argv;
      this->FileName = fname;
  }

  void CreatePipeline()
  {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    this->Reader = vtkXdmf3Reader::New();
    this->Reader->SetFileName(this->FileName.c_str());
    cerr << my_id << "/" << num_procs << " " << this->FileName << endl;
  }

protected:
  MyProcess() { this->Argc = 0; this->Argv = NULL;}

  int Argc;
  char **Argv;
  std::string FileName;
  vtkXdmf3Reader *Reader;
};

vtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  int proc = this->Controller->GetLocalProcessId();
  int numprocs = this->Controller->GetNumberOfProcesses();

  this->Controller->Barrier();
  this->CreatePipeline();
  this->Controller->Barrier();
  this->Reader->UpdatePiece(proc, numprocs, 0);
  this->Reader->Delete();
  this->ReturnValue = 1;
}


int main(int argc, char *argv[])
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

  int retVal = 1; // 1 == failed

  int numProcs = contr->GetNumberOfProcesses();

  if (numProcs < 2 && false)
  {
    cout << "This test requires at least 2 processes" << endl;
    contr->Delete();
    return retVal;
  }

  vtkMultiProcessController::SetGlobalController(contr);

  vtkTesting *testHelper = vtkTesting::New();
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  std::string datadir = testHelper->GetDataRoot();
  std::string file = datadir + "/Data/XDMF/Iron/Iron_Protein.ImageData.xmf";
  cerr << file << endl;
  testHelper->Delete();

  //allow caller to use something else
  for (int i = 0; i<argc; i++)
  {
    if (!strncmp(argv[i], "--file=", 11))
    {
      file=argv[i]+11;
    }
  }
  MyProcess *p = MyProcess::New();
  p->SetArgs(argc, argv, file.c_str());

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  vtkMultiProcessController::SetGlobalController(0);
  return !retVal;
}
