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
// This test exercises xdmf3 reading and writing in parallel.
//

#include <vtk_mpi.h>

#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkProcess.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXdmf3Reader.h"
#include "vtkXdmf3Writer.h"

#include <vtksys/SystemTools.hxx>

class MyProcess : public vtkProcess
{
public:
  static MyProcess* New();
  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute() override;

  void SetArgs(int argc, char* argv[], const std::string& ifname, const std::string& ofname)
  {
    this->Argc = argc;
    this->Argv = argv;
    this->InFileName = ifname;
    this->OutFileName = ofname;
  }

  void CreatePipeline()
  {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    this->Reader = vtkXdmf3Reader::New();
    this->Reader->SetFileName(this->InFileName.c_str());
    if (my_id == 0)
    {
      cerr << my_id << "/" << num_procs << endl;
      cerr << "IFILE " << this->InFileName << endl;
      cerr << "OFILE " << this->OutFileName << endl;
    }

    this->Writer = vtkXdmf3Writer::New();
    this->Writer->SetFileName(this->OutFileName.c_str());
    this->Writer->SetInputConnection(this->Reader->GetOutputPort());
  }

protected:
  MyProcess()
  {
    this->Argc = 0;
    this->Argv = nullptr;
  }

  int Argc;
  char** Argv;
  std::string InFileName;
  std::string OutFileName;
  vtkXdmf3Reader* Reader;
  vtkXdmf3Writer* Writer;
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
  this->Writer->Write();
  this->Reader->Delete();
  this->Writer->Delete();
  this->ReturnValue = 1;
}

int TestXdmf3Parallel(int argc, char** argv)
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController* contr = vtkMPIController::New();
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

  vtkTesting* testHelper = vtkTesting::New();
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  std::string datadir = testHelper->GetDataRoot();
  std::string ifile = datadir + "/Data/XDMF/Iron/Iron_Protein.ImageData.xmf";
  std::string tempdir = testHelper->GetTempDirectory();
  tempdir = tempdir + "/XDMF";
  vtksys::SystemTools::MakeDirectory(tempdir.c_str());
  std::string ofile = tempdir + "/Iron_Protein.ImageData.xmf";
  testHelper->Delete();

  // allow caller to use something else
  for (int i = 0; i < argc; i++)
  {
    if (!strncmp(argv[i], "--file=", 11))
    {
      ifile = argv[i] + 11;
    }
  }
  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv, ifile.c_str(), ofile.c_str());

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  vtkMultiProcessController::SetGlobalController(0);

  if (retVal)
  {
    // test passed, remove the files we wrote
    vtksys::SystemTools::RemoveADirectory(tempdir.c_str());
  }
  return !retVal;
}
