/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SocketClient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRectilinearGrid.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "ExerciseMultiProcessController.h"

static const int scMsgLength = 10;

static void CleanUp(vtkSmartPointer<vtkSocketCommunicator> vtkNotUsed(comm),
                    vtkSmartPointer<vtkSocketController> vtkNotUsed(contr))
{
  // This will close the connection as well as delete 
  // the communicator
  // Deleting no longer necessary with smart pointers.
//   comm->Delete();
//   contr->Delete();
}

int main(int argc, char** argv)
{
  VTK_CREATE(vtkSocketController, contr);
  contr->Initialize();

  VTK_CREATE(vtkSocketCommunicator, comm);

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
  VTK_CREATE(vtkUnstructuredGrid, ugrid);

  if (!comm->Receive(ugrid, 1, 9))
    {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  VTK_CREATE(vtkDataSetMapper, umapper);
  umapper->SetInput(ugrid);

  VTK_CREATE(vtkActor, uactor);
  uactor->SetMapper(umapper);
  uactor->SetPosition(5, 0, 0);
  uactor->SetScale(0.2, 0.2, 0.2);

  // Test receiving vtkDataArray
  VTK_CREATE(vtkDoubleArray, da);
  if (!comm->Receive(da, 1, 9))
    {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
    }
  for (i=0; i<40; i++)
    {
    if (da->GetValue(i) != static_cast<double>(i))
      {
      cerr << "Server error: Corrupt vtkDoubleArray." << endl;
      CleanUp(comm, contr);
      return 1;
      }
    }

  // Test receiving null vtkDataArray
  VTK_CREATE(vtkDoubleArray, da2);
  if (!comm->Receive(da2, 1, 9))
    {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
    }
  if (da2->GetNumberOfTuples() == 0) 
    {
    cout << "receive null data array successful" << endl;
    } 
  else 
    {
    cout << "receive null data array failed" << endl;
    }

  contr->SetCommunicator(comm);

  // The following lines were added for coverage
  // These methods have empty implementations
  contr->SingleMethodExecute();
  contr->MultipleMethodExecute();
  contr->CreateOutputWindow();
  contr->Barrier();
  contr->Finalize();

  // Run the socket through the standard controller tests.  We have to make a
  // compliant controller first.
  int retVal;
  vtkMultiProcessController *compliantController
    = contr->CreateCompliantController();
  retVal = ExerciseMultiProcessController(compliantController);
  compliantController->Delete();
  if (retVal)
    {
    CleanUp(comm, contr);
    return retVal;
    }

  VTK_CREATE(vtkPolyDataMapper, pmapper);
  VTK_CREATE(vtkPolyData, pd);
  comm->Receive(pd, 1, 11);
  pmapper->SetInput(pd);

  VTK_CREATE(vtkActor, pactor);
  pactor->SetMapper(pmapper);

  VTK_CREATE(vtkDataSetMapper, rgmapper);
  VTK_CREATE(vtkRectilinearGrid, rg);
  comm->Receive(rg, 1, 11);
  rgmapper->SetInput(rg);

  VTK_CREATE(vtkActor, rgactor);
  rgactor->SetMapper(rgmapper);
  rgactor->SetPosition(0, -5, 0);
  rgactor->SetScale(2, 2, 2);

  VTK_CREATE(vtkContourFilter, iso2);
  VTK_CREATE(vtkStructuredGrid, sg);
  comm->Receive(sg, 1, 11);
  iso2->SetInput(sg);
  iso2->SetValue(0, .205);

  VTK_CREATE(vtkPolyDataMapper, sgmapper);
  sgmapper->SetInputConnection(0, iso2->GetOutputPort());

  VTK_CREATE(vtkActor, sgactor);
  sgactor->SetMapper(sgmapper);
  sgactor->SetPosition(10, -5, -40);


  VTK_CREATE(vtkImageData, id);
  comm->Receive(id, 1, 11);

  VTK_CREATE(vtkImageActor, imactor);
  imactor->SetInput(id);
  imactor->SetPosition(10, 0, 10);
  imactor->SetScale(0.02, 0.02, 0.02);

  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(uactor);
  ren->AddActor(pactor);
  ren->AddActor(rgactor);
  ren->AddActor(sgactor);
  ren->AddActor(imactor);

  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->SetSize(500,400);
  renWin->AddRenderer(ren);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(2.2);

  renWin->Render();

  retVal = vtkRegressionTestImage( renWin );

  CleanUp(comm, contr);

  return !retVal;
}
