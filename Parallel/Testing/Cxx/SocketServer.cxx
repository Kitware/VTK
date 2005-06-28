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
#include "vtkOutputPort.h"
#include "vtkPLOT3DReader.h"
#include "vtkPNMReader.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkTestUtilities.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

static const int scMsgLength = 10;

static void CleanUp(vtkSocketCommunicator* comm, vtkSocketController* contr)
{
  comm->CloseConnection();
  comm->Delete();
  contr->Delete();
}

int main(int argc, char** argv)
{
#if !defined(VTK_LEGACY_REMOVE) && defined(VTK_LEGACY_SILENT)
  vtkDebugLeaks::PromptUserOff();
#endif

  vtkSocketController* contr = vtkSocketController::New();
  contr->Initialize();

  vtkSocketCommunicator* comm = vtkSocketCommunicator::New();

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
    comm->Delete();
    contr->Delete();
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
  vtkUnstructuredGridReader* ugrid = vtkUnstructuredGridReader::New();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, 
                                                     "Data/blow.vtk");
  ugrid->SetFileName(fname);
  delete[] fname;

  ugrid->Update();

  if (!comm->Send(ugrid->GetOutput(), 1, 9))
    {
    cerr << "Server error: Error sending data." << endl;
    CleanUp(comm, contr);
    ugrid->Delete();
    return 1;
    }
  ugrid->Delete();

  // Test receiving vtkDataArray
  vtkDoubleArray* da = vtkDoubleArray::New();
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
    da->Delete();
    return 1;
    }
  da->Delete();

  // Test receiving null vtkDataArray
  vtkDoubleArray *da2 = NULL;
  if (!comm->Send(da2, 1, 9))
    {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
    }

  contr->SetCommunicator(comm);

  vtkOutputPort* op = vtkOutputPort::New();
  op->SetController(contr);
  op->SetTag(45);

  vtkBYUReader* pd = vtkBYUReader::New();
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  pd->SetGeometryFileName(fname);
  delete[] fname;

  pd->Update();

  op->SetInput(pd->GetOutput());
  op->WaitForUpdate();
  pd->Delete();

  vtkRectilinearGridReader* rgrid = vtkRectilinearGridReader::New();
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, 
                                               "Data/RectGrid2.vtk");
  rgrid->SetFileName(fname);
  delete[] fname;

  rgrid->Update();

  op->SetInput(rgrid->GetOutput());
  op->WaitForUpdate();
  rgrid->Delete();

  vtkStructuredPointsReader* spgrid = vtkStructuredPointsReader::New();
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, 
                                               "Data/ironProt.vtk");
  spgrid->SetFileName(fname);
  delete[] fname;

  spgrid->Update();

  op->SetInput(spgrid->GetOutput());
  op->WaitForUpdate();
  spgrid->Delete();

  vtkPLOT3DReader* pl3d = vtkPLOT3DReader::New();
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  pl3d->SetXYZFileName(fname);
  delete[] fname;
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");
  pl3d->SetQFileName(fname);
  delete[] fname;
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);

  pl3d->Update();

  op->SetInput(pl3d->GetOutput());
  op->WaitForUpdate();
  pl3d->Delete();

  vtkPNMReader* imageData = vtkPNMReader::New();
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/earth.ppm");
  imageData->SetFileName(fname);
  delete[] fname;

  imageData->Update();

  op->SetInput(imageData->GetOutput());
  op->WaitForUpdate();
  imageData->Delete();

  op->Delete();
  CleanUp(comm, contr);
  return 0;
}
