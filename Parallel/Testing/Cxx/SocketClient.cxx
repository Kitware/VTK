#include "vtkSocketController.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkDoubleArray.h"
#include "vtkRegressionTestImage.h"
#include "vtkInputPort.h"
#include "vtkPolyDataMapper.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"

static const int scMsgLength = 10;

static void CleanUp(vtkSocketCommunicator* comm, vtkSocketController* contr)
{
  // This will close the connection as well as delete 
  // the communicator
  comm->Delete();
  contr->Delete();
}

int main(int argc, char** argv)
{
  vtkDebugLeaks::PromptUserOff();

  vtkSocketController* contr = vtkSocketController::New();
  contr->Initialize();

  vtkSocketCommunicator* comm = vtkSocketCommunicator::New();

  int i;

  // Get the host name from the command line arguments
  char* hostname = new char[30];
  strcpy(hostname, "localhost");
  int dataIndex=-1;
  for (i=0; i<argc; i++)
    {
    if ( strcmp("-H", argv[i]) == 0 )
      {
      if ( i < argc-1 )
	{
	dataIndex = i+1;
	}
      }
    }
  if (dataIndex != -1)
    {
    delete[] hostname;
    hostname = new char[strlen(argv[dataIndex])+1];
    strcpy(hostname, argv[dataIndex]);
    }

  // Get the port from the command line arguments
  int port=11111;

  dataIndex=-1;
  for (i=0; i<argc; i++)
    {
    if ( strcmp("-P", argv[i]) == 0 )
      {
      if ( i < argc-1 )
	{
	dataIndex = i+1;
	}
      }
    }
  if (dataIndex != -1)
    {
    port = atoi(argv[dataIndex]);
    }

  // Establish connection
  if (!comm->ConnectTo(hostname, port))
    {
    cerr << "Client error: Could not connect to the server."
	 << endl;
    comm->Delete();
    contr->Delete();
    delete[] hostname;
    return 1;
    }
  delete[] hostname;

  // Test sending all supported types of arrays
  int datai[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datai[i] = i;
    }
  if (!comm->Send(datai, scMsgLength, 1, 11))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  unsigned long dataul[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataul[i] = static_cast<unsigned long>(i);
    }
  if (!comm->Send(dataul, scMsgLength, 1, 22))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  char datac[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datac[i] = static_cast<char>(i);
    }
  if (!comm->Send(datac, scMsgLength, 1, 33))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  unsigned char datauc[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datauc[i] = static_cast<unsigned char>(i);
    }
  if (!comm->Send(datauc, scMsgLength, 1, 44))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  float dataf[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    dataf[i] = static_cast<float>(i);
    }
  if (!comm->Send(dataf, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }


  double datad[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datad[i] = static_cast<double>(i);
    }
  if (!comm->Send(datad, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  vtkIdType datait[scMsgLength];
  for (i=0; i<scMsgLength; i++)
    {
    datait[i] = static_cast<vtkIdType>(i);
    }
  if (!comm->Send(datait, scMsgLength, 1, 7))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  // Test receiving vtkDataObject
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();

  if (!comm->Receive(ugrid, 1, 9))
    {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    ugrid->Delete();
    return 1;
    }

  vtkDataSetMapper* umapper = vtkDataSetMapper::New();
  umapper->SetInput(ugrid);

  vtkActor* uactor = vtkActor::New();
  uactor->SetMapper(umapper);
  uactor->SetPosition(5, 0, 0);
  uactor->SetScale(0.2, 0.2, 0.2);
  umapper->UnRegister(0);

  // Test receiving vtkDataArray
  vtkDoubleArray* da = vtkDoubleArray::New();
  if (!comm->Receive(da, 1, 9))
    {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    ugrid->Delete();
    uactor->Delete();
    da->Delete();
    return 1;
    }
  for (i=0; i<40; i++)
    {
    if (da->GetValue(i) != static_cast<double>(i))
      {
      cerr << "Server error: Corrupt vtkDoubleArray." << endl;
      CleanUp(comm, contr);
      ugrid->Delete();
      uactor->Delete();
      da->Delete();
      return 1;
      }
    }

  da->Delete();

  contr->SetCommunicator(comm);

  // Test the ports
  vtkInputPort* ip = vtkInputPort::New();
  ip->SetController(contr);
  ip->SetTag(45);
  ip->SetRemoteProcessId(1);

  // Get polydata
  ip->GetPolyDataOutput()->Update();

  vtkPolyDataMapper* pmapper = vtkPolyDataMapper::New();
  pmapper->SetInput(ip->GetPolyDataOutput());
  // Break the connection and tell the server to fall out of loop
  ip->GetPolyDataOutput()->SetSource(0);
  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  vtkActor* pactor = vtkActor::New();
  pactor->SetMapper(pmapper);
  pmapper->UnRegister(0);

  // Get rectilinear grid
  ip->GetRectilinearGridOutput()->Update();

  vtkDataSetMapper* rgmapper = vtkDataSetMapper::New();
  rgmapper->SetInput(ip->GetRectilinearGridOutput());
  ip->GetRectilinearGridOutput()->SetSource(0);
  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  vtkActor* rgactor = vtkActor::New();
  rgactor->SetMapper(rgmapper);
  rgactor->SetPosition(0, -5, 0);
  rgactor->SetScale(2, 2, 2);
  rgmapper->UnRegister(0);

  // Get structured points
  ip->GetStructuredPointsOutput()->Update();

  vtkContourFilter* iso = vtkContourFilter::New();
  iso->SetInput(ip->GetStructuredPointsOutput());
  iso->SetValue(0, 128);
  ip->GetStructuredPointsOutput()->SetSource(0);
  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  vtkPolyDataMapper* spmapper = vtkPolyDataMapper::New();
  spmapper->SetInput(iso->GetOutput());
  iso->UnRegister(0);

  vtkActor* spactor = vtkActor::New();
  spactor->SetMapper(spmapper);
  spactor->SetPosition(5, -5, 0);
  spactor->SetScale(0.1, 0.1, 0.1);
  spmapper->UnRegister(0);

  // Get structured grid
  ip->GetStructuredGridOutput()->Update();

  vtkContourFilter* iso2 = vtkContourFilter::New();
  iso2->SetInput(ip->GetStructuredGridOutput());
  iso2->SetValue(0, .205);
  ip->GetStructuredGridOutput()->SetSource(0);
  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  vtkPolyDataMapper* sgmapper = vtkPolyDataMapper::New();
  sgmapper->SetInput(iso2->GetOutput());
  iso2->UnRegister(0);

  vtkActor* sgactor = vtkActor::New();
  sgactor->SetMapper(sgmapper);
  sgactor->SetPosition(10, -5, 0);
  sgmapper->UnRegister(0);

  // Get image data
  ip->GetImageDataOutput()->Update();

  vtkContourFilter* iso3 = vtkContourFilter::New();
  iso3->SetInput(ip->GetImageDataOutput());
  iso3->SetValue(0, .205);
  ip->GetImageDataOutput()->SetSource(0);
  contr->TriggerRMI(1, vtkMultiProcessController::BREAK_RMI_TAG);

  vtkPolyDataMapper* immapper = vtkPolyDataMapper::New();
  immapper->SetInput(iso3->GetOutput());
  iso3->UnRegister(0);

  vtkActor* imactor = vtkActor::New();
  imactor->SetMapper(immapper);
  imactor->SetPosition(10, 0, 0);
  imactor->SetScale(0.02, 0.02, 0.02);
  immapper->UnRegister(0);

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(uactor);
  ren->AddActor(pactor);
  ren->AddActor(rgactor);
  ren->AddActor(spactor);
  ren->AddActor(sgactor);
  ren->AddActor(imactor);
  uactor->UnRegister(0);
  pactor->UnRegister(0);
  rgactor->UnRegister(0);
  spactor->UnRegister(0);
  sgactor->UnRegister(0);
  imactor->UnRegister(0);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  ren->UnRegister(0);

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  ip->Delete();
  renWin->Delete();
  ugrid->Delete();
  CleanUp(comm, contr);

  return !retVal;
}
