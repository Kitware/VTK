#include "vtkMPIController.h"

int main( int argc, char *argv[] )
{
  vtkMPIController *controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv);

  vtkMPICommunicator::Request req;
  int val;
  
  if (controller->GetLocalProcessId() == 0)
    {
    val = 12;
    controller->NoBlockSend(&val, 1, 1, 99, req);
    }
  else
    {
    controller->NoBlockReceive(&val, 1, vtkMultiProcessController::ANY_SOURCE,
			       99, req);
    if (req.Test())
      cout << "Receive succeeded: " << val << endl;
    else
      cout << "Receive failed." << endl;
    }

  controller->Finalize();
  controller->Delete();
}
