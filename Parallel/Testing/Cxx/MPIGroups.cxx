#include "vtkMPIController.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTreeComposite.h"
#include "vtkDebugLeaks.h"
#include "vtkParallelFactory.h"
#include "vtkRegressionTestImage.h"

static const int NUM_PROC_PER_GROUP = 2;

struct MPIGroupsArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void proc1( vtkMultiProcessController *controller, void *arg );
void proc2( vtkMultiProcessController *controller, void *arg );

void MyMain( vtkMultiProcessController *controller, void *arg )
{

  MPIGroupsArgs_tmp* args = 
    reinterpret_cast<MPIGroupsArgs_tmp*>(arg);

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  vtkMPICommunicator* localComm = vtkMPICommunicator::New();
  vtkMPIGroup* localGroup= vtkMPIGroup::New();
  vtkMPIController* localController = vtkMPIController::New();
  vtkMPICommunicator* worldComm = vtkMPICommunicator::GetWorldCommunicator();

  int numGroups = numProcs / NUM_PROC_PER_GROUP;
  int currentGroup = myId / NUM_PROC_PER_GROUP;

  localGroup->Initialize( static_cast<vtkMPIController*>(controller) );
  for(int i=0; i<NUM_PROC_PER_GROUP; i++)
    {
    localGroup->AddProcessId(currentGroup*NUM_PROC_PER_GROUP + i);
    }
  localComm->Initialize(worldComm, localGroup);
  localGroup->UnRegister(0);

  // Create a local controller (for the sub-group)
  localController->SetCommunicator(localComm);
  localComm->UnRegister(0);

  if ( currentGroup == 0 )
    {
    localController->SetSingleMethod(proc1, arg);
    localController->SingleMethodExecute();
    }
  else if ( currentGroup == 1 )
    {
    localController->SetSingleMethod(proc2, 0);
    localController->SingleMethodExecute();
    }
  localController->Delete();

}

// This will be called by all processes
void proc1( vtkMultiProcessController *controller, void *arg )
{

  MPIGroupsArgs_tmp* args = 
    reinterpret_cast<MPIGroupsArgs_tmp*>(arg);

  int myid, numProcs;
  float val;
  int numTris;

  // Obtain the id of the running process and the total
  // number of processes
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);

  vtkPolyDataNormals* pdn = vtkPolyDataNormals::New();
  pdn->SetInput(sphere->GetOutput());
  sphere->UnRegister(0);

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(pdn->GetOutput());
  pdn->UnRegister(0);

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  mapper->UnRegister(0);

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor(actor);
  actor->UnRegister(0);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  ren->UnRegister(0);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // The only thing we have to do to get parallel execution.
  vtkTreeComposite*  treeComp = vtkTreeComposite::New();
  treeComp->SetController(controller);
  treeComp->SetRenderWindow(renWin);

  // Tell the mappers to only update a piece (based on process) of 
  // their inputs.
  treeComp->InitializePieces();

  if (myid)
    {
    treeComp->InitializeRMIs();
    controller->ProcessRMIs();
    controller->Receive(args->retVal, 1, 0, 33);
    }
  else
    {
    renWin->Render();
    renWin->Render();
    *(args->retVal) = 
      vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);
    for (int i = 1; i < numProcs; i++)
      {
      controller->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG);
      controller->Send(args->retVal, 1, i, 33);
      }
    }

  if (!myid)
    {
    }

  if ( *(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  iren->Delete();
  renWin->Delete();
  treeComp->Delete();
}

void proc2( vtkMultiProcessController *contr, void *arg )
{
  vtkMPIController* controller = static_cast<vtkMPIController*>(contr);

  if (controller->GetLocalProcessId() == 0)
    {
    int vali = 12;
    vtkMPICommunicator::Request req1;
    controller->NoBlockSend(&vali, 1, 1, 95, req1);
    int valul = 12;
    vtkMPICommunicator::Request req2;
    controller->NoBlockSend(&valul, 1, 1, 96, req2);
    int valc = 12;
    vtkMPICommunicator::Request req3;
    controller->NoBlockSend(&valc, 1, 1, 97, req3);
    int valf = 12;
    vtkMPICommunicator::Request req4;
    controller->NoBlockSend(&valf, 1, 1, 98, req4);
    }
  else if (controller->GetLocalProcessId() == 1)
    {
    int vali;
    vtkMPICommunicator::Request req1;
    controller->NoBlockReceive(&vali, 1, vtkMultiProcessController::ANY_SOURCE,
			       95, req1);
    if (req1.Test() && (vali == 12))
      cout << "Receive (int) succeeded." << endl;
    else
      cout << "Receive (int) failed:" << vali << endl;
    int valul;
    vtkMPICommunicator::Request req2;
    controller->NoBlockReceive(&valul, 1, vtkMultiProcessController::ANY_SOURCE,
			       96, req2);
    if (req2.Test() && (valul == 12))
      cout << "Receive (unsigned long) succeeded." << endl;
    else
      cout << "Receive (unsigned long) failed:" << valul << endl;
    int valc;
    vtkMPICommunicator::Request req3;
    controller->NoBlockReceive(&valc, 1, vtkMultiProcessController::ANY_SOURCE,
			       97, req3);
    if (req3.Test() && (valc == 12))
      cout << "Receive (char) succeeded." << endl;
    else
      cout << "Receive (char) failed:" << valc  << endl;
    int valf;
    vtkMPICommunicator::Request req4;
    controller->NoBlockReceive(&valf, 1, vtkMultiProcessController::ANY_SOURCE,
			       98, req4);
    if (req4.Test() && (valf == 12))
      cout << "Receive (float) succeeded." << endl;
    else
      cout << "Receive (float) failed:" << valf  << endl;
    }

  // Just for coverage
  controller->Barrier();

}

int main( int argc, char* argv[] )
{
  vtkMPIController *controller;

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkMPIController otherwise.
  controller = vtkMPIController::New();

  vtkDebugLeaks::PromptUserOff();
  controller->Initialize(&argc, &argv);
  vtkDebugLeaks::PromptUserOff();

  vtkParallelFactory* pf = vtkParallelFactory::New();
  vtkObjectFactory::RegisterFactory(pf);
  pf->Delete();
 
  // Added for regression test.
  // ----------------------------------------------
  int retVal;
  MPIGroupsArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  controller->SetSingleMethod(MyMain, &args);
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();

  return !retVal;
}





