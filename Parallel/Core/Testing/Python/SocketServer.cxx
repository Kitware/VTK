/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SocketServer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBYUReader.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPNMReader.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkTestUtilities.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkDataSetReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "ExerciseMultiProcessController.h"

static const int scMsgLength = 10;

static void CleanUp(vtkSmartPointer<vtkSocketCommunicator> comm,
                    vtkSmartPointer<vtkSocketController> vtkNotUsed(contr))
{
  comm->CloseConnection();
  // Deleting no longer necessary with smart pointers.
//   comm->Delete();
//   contr->Delete();
}

int main(int argc, char** argv)
{
  VTK_CREATE(vtkSocketController, contr);
  contr->Initialize();

  VTK_CREATE(vtkSocketCommunicator, comm);

  int port=11111;

  int i;

  // Get the port from the command line arguments
  int dataIndex=-1;
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
  if (!comm->WaitForConnection(port))
  {
    cerr << "Server error: Wait timed out or could not initialize socket."
         << endl;
    return 1;
  }


  // Test receiving all supported types of arrays
  int datai[scMsgLength];
  if (!comm->Receive(datai, scMsgLength, 1, 11))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (datai[i] != i)
    {
      cerr << "Server error: Corrupt integer array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  unsigned long dataul[scMsgLength];
  if (!comm->Receive(dataul, scMsgLength, 1, 22))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (dataul[i] != static_cast<unsigned long>(i))
    {
      cerr << "Server error: Corrupt unsigned long array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  char datac[scMsgLength];
  if (!comm->Receive(datac, scMsgLength, 1, 33))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (datac[i] != static_cast<char>(i))
    {
      cerr << "Server error: Corrupt char array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  unsigned char datauc[scMsgLength];
  if (!comm->Receive(datauc, scMsgLength, 1, 44))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (datauc[i] != static_cast<unsigned char>(i))
    {
      cerr << "Server error: Corrupt unsigned char array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  float dataf[scMsgLength];
  if (!comm->Receive(dataf, scMsgLength, 1, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (dataf[i] != static_cast<float>(i))
    {
      cerr << "Server error: Corrupt float array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }


  double datad[scMsgLength];
  if (!comm->Receive(datad, scMsgLength, 1, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (datad[i] != static_cast<double>(i))
    {
      cerr << "Server error: Corrupt double array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  vtkIdType datait[scMsgLength];
  if (!comm->Receive(datait, scMsgLength, 1, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i=0; i<scMsgLength; i++)
  {
    if (datait[i] != static_cast<vtkIdType>(i))
    {
      cerr << "Server error: Corrupt vtkIdType array." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }


  // Test sending vtkDataObject
  VTK_CREATE(vtkUnstructuredGridReader, ugrid);
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                     "Data/blow.vtk");
  ugrid->SetFileName(fname);
  delete[] fname;

  ugrid->Update();

  if (!comm->Send(ugrid->GetOutput(), 1, 9))
  {
    cerr << "Server error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  // Test receiving vtkDataArray
  VTK_CREATE(vtkDoubleArray, da);
  da->SetNumberOfComponents(4);
  da->SetNumberOfTuples(10);
  for(i=0; i<40; i++)
  {
    da->SetValue(i,static_cast<double>(i));
  }
  if (!comm->Send(da, 1, 9))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  // Test receiving null vtkDataArray
  vtkDoubleArray *da2 = NULL;
  if (!comm->Send(da2, 1, 9))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  contr->SetCommunicator(comm);

  // The following lines were added for coverage
  // These methods have empty implementations
  contr->SingleMethodExecute();
  contr->MultipleMethodExecute();
  contr->CreateOutputWindow();
  contr->Barrier();
  contr->Finalize();

  // First, run the socket through the standard controller tests.  We have
  // to make a compliant controller first.
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

  VTK_CREATE(vtkBYUReader, pd);
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  pd->SetGeometryFileName(fname);
  delete[] fname;

  pd->Update();

  comm->Send(pd->GetOutput(), 1, 11);

  VTK_CREATE(vtkRectilinearGridReader, rgrid);
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                               "Data/RectGrid2.vtk");
  rgrid->SetFileName(fname);
  delete[] fname;

  rgrid->Update();

  comm->Send(rgrid->GetOutput(), 1, 11);

  VTK_CREATE(vtkMultiBlockPLOT3DReader, pl3d);
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  pl3d->SetXYZFileName(fname);
  delete[] fname;
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");
  pl3d->SetQFileName(fname);
  delete[] fname;
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);

  pl3d->Update();

  comm->Send(pl3d->GetOutput()->GetBlock(0), 1, 11);

  VTK_CREATE(vtkPNMReader, imageData);
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/earth.ppm");
  imageData->SetFileName(fname);
  delete[] fname;

  imageData->Update();

  comm->Send(imageData->GetOutput(), 1, 11);

  CleanUp(comm, contr);
  return 0;
}
