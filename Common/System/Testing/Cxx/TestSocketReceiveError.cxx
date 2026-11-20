// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Check that vtkSocket::Receive stops on a failed recv() instead of adding
// the -1 to its byte count and writing before the caller's buffer on the
// next call.

#include "vtkClientSocket.h"
#include "vtkNew.h"
#include "vtkServerSocket.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/socket.h>
#include <sys/time.h>
#endif

int TestSocketReceiveError(int, char*[])
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  // The receive timeout used to make recv() fail is set up differently on
  // Windows, and the Windows code path already checked for errors.
  return EXIT_SUCCESS;
#else
  vtkNew<vtkServerSocket> server;
  if (server->CreateServer(0) != 0)
  {
    std::cerr << "Could not create a server socket.\n";
    return EXIT_FAILURE;
  }
  const int port = server->GetServerPort();

  // The server sends 2 bytes, waits longer than the client's receive
  // timeout, then sends 6 more.
  std::thread sender(
    [&server]()
    {
      vtkSmartPointer<vtkClientSocket> connection;
      connection.TakeReference(server->WaitForConnection(5000));
      if (!connection)
      {
        return;
      }
      connection->Send("AB", 2);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      connection->Send("CDEFGH", 6);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    });

  vtkNew<vtkClientSocket> client;
  if (client->ConnectToServer("127.0.0.1", port) != 0)
  {
    std::cerr << "Could not connect to the server.\n";
    sender.join();
    return EXIT_FAILURE;
  }

  // A 100 ms receive timeout makes recv() return -1 while the server waits.
  timeval timeout{ 0, 100000 };
  setsockopt(client->GetSocketDescriptor(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  vtkNew<vtkTest::ErrorObserver> errorObserver;
  client->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  struct
  {
    char Guard[16];
    char Buffer[8];
    char Tail[8];
  } memory;
  std::memset(&memory, '.', sizeof(memory));

  const int received = client->Receive(memory.Buffer, 8, 1);
  sender.join();

  int status = EXIT_SUCCESS;
  for (char c : memory.Guard)
  {
    if (c != '.')
    {
      std::cerr << "Receive wrote before the buffer: '" << std::string(memory.Guard, 16) << "'\n";
      status = EXIT_FAILURE;
      break;
    }
  }
  if (std::string(memory.Buffer, 2) != "AB")
  {
    std::cerr << "Expected the buffer to start with \"AB\", got '" << std::string(memory.Buffer, 8)
              << "'\n";
    status = EXIT_FAILURE;
  }
  if (received != 0)
  {
    std::cerr << "Expected Receive to return 0 after a failed recv(), got " << received << "\n";
    status = EXIT_FAILURE;
  }
  if (!errorObserver->GetError())
  {
    std::cerr << "Expected Receive to report an error.\n";
    status = EXIT_FAILURE;
  }
  return status;
#endif
}
