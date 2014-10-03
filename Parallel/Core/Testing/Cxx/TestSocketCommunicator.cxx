/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSocketCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test tests vtkSocketCommunicator.
#include "vtkNew.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkTesting.h"
#include "vtkServerSocket.h"
#include "vtkPolyData.h"
#include "vtkDoubleArray.h"

#include <vtksys/ios/sstream>

#define MESSAGE(x)\
  cout << (is_server? "SERVER" : "CLIENT") << ":" x << endl;

int main(int argc, char *argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  bool is_server = false;
  for (int cc=1; cc < argc; cc++)
    {
    if (argv[cc] &&
        (strcmp(argv[cc], "--server") == 0||
         strcmp(argv[cc], "\"--server\"") == 0))
      {
      is_server = true;
      break;
      }
    }

  vtksys_ios::ostringstream stream;
  stream << testing->GetTempDirectory() << "/TestSocketCommunicator."
    << (is_server? "server" : "client") << ".log";
  // initialize the socket controller.
  vtkNew<vtkSocketController> controller;
  controller->Initialize(&argc, &argv);

  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
    controller->GetCommunicator());
  comm->SetReportErrors(1);
  comm->LogToFile(stream.str().c_str());
  if (is_server)
    {
    MESSAGE("Waiting on 10240");
    controller->WaitForConnection(10240);
    }
  else
    {
    MESSAGE("Connecting to 10240");
    controller->ConnectTo("localhost", 10240);
    }
  if (!comm->Handshake())
    {
    return EXIT_FAILURE;
    }
  MESSAGE("Connected.");

  int idata = 0;
  double ddata = 0;
  vtkNew<vtkDoubleArray> dArray;
  vtkNew<vtkPolyData> pData;

  for (int cc=0; cc < 2; cc++)
    {
    MESSAGE("---- Test stage " << cc << "----");
    if (is_server)
      {
      idata = 10;
      ddata = 10.0;
      dArray->SetNumberOfTuples(10);
      dArray->FillComponent(0, 10.0);
      pData->Initialize();

      controller->Send(&idata, 1, 1, 101011);
      controller->Send(&ddata, 1, 1, 101012);
      controller->Send(dArray.GetPointer(), 1, 101013);
      controller->Send(pData.GetPointer(), 1, 101014);
      }
    else
      {
      controller->Receive(&idata, 1, 1, 101011);
      controller->Receive(&ddata, 1, 1, 101012);
      controller->Receive(dArray.GetPointer(), 1, 101013);
      controller->Receive(pData.GetPointer(), 1, 101014);
      if (idata != 10 ||
        ddata != 10.0 ||
        dArray->GetNumberOfTuples() != 10 ||
        dArray->GetValue(9) != 10.0)
        {
        MESSAGE("ERROR: Communication failed!!!");
        return EXIT_FAILURE;
        }
      }
    MESSAGE("   .... PASSED!");
    // switch the flags so server becomes client and client becomes server and
    // ship messages around.
    is_server = !is_server;
    }
  MESSAGE("All's well!");
  return EXIT_SUCCESS;
}
