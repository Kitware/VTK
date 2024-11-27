// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test tests vtkSocketCommunicator.
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkServerSocket.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkTesting.h"

#include <sstream>

#define MESSAGE(x) cout << (is_server ? "SERVER" : "CLIENT") << ":" x << endl;

//-----------------------------------------------------------------------------
// This unit test make sure that we can Send/Receive a int, vtkDataArray and a vtkDataSet
bool TestSendReceiveDataArray(vtkSocketController* controller, bool& is_server)
{
  int idata = 0;
  double ddata = 0;
  vtkNew<vtkDoubleArray> dArray;
  vtkNew<vtkPolyData> pData;

  MESSAGE("---- TestSendReceiveDataArray ----")

  for (int cc = 0; cc < 2; cc++)
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
      controller->Send(dArray, 1, 101013);
      controller->Send(pData, 1, 101014);
    }
    else
    {
      controller->Receive(&idata, 1, 1, 101011);
      controller->Receive(&ddata, 1, 1, 101012);
      controller->Receive(dArray, 1, 101013);
      controller->Receive(pData, 1, 101014);
      if (idata != 10 || ddata != 10.0 || dArray->GetNumberOfTuples() != 10 ||
        dArray->GetValue(9) != 10.0)
      {
        MESSAGE("ERROR: Communication failed!!!");
        return false;
      }
    }
    MESSAGE("   .... PASSED!");
    // switch the flags so server becomes client and client becomes server and
    // ship messages around.
    is_server = !is_server;
  }
  MESSAGE("All's well!");

  return true;
}

//-----------------------------------------------------------------------------
// This unit test make sure that it is correctly checked, especially on Windows as it previously the
// server hangs indefinitely.
bool TestConnectionAbortHandling(vtkSocketController* controller, bool& is_server)
{
  MESSAGE("---- TestConnectionAbortHandling ----")
  MESSAGE("Check support of connection failure...")
  vtkNew<vtkDoubleArray> dArray;

  if (is_server)
  {
    controller->Receive(dArray, 1, 101013);
    MESSAGE("Error is expected, continue.")
  }
  else
  {
    MESSAGE("Kill the client")
    controller->TriggerBreakRMIs();
  }

  return true;
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  bool is_server = false;
  for (int cc = 1; cc < argc; cc++)
  {
    if (argv[cc] && (strcmp(argv[cc], "--server") == 0 || strcmp(argv[cc], "\"--server\"") == 0))
    {
      is_server = true;
      break;
    }
  }

  std::ostringstream stream;
  stream << testing->GetTempDirectory() << "/TestSocketCommunicator."
         << (is_server ? "server" : "client") << ".log";
  // initialize the socket controller.
  vtkNew<vtkSocketController> controller;
  controller->Initialize(&argc, &argv);

  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator());
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

  bool succeed = true;
  succeed &= ::TestSendReceiveDataArray(controller, is_server);
  succeed &= ::TestConnectionAbortHandling(controller, is_server);

  if (succeed)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
