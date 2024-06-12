// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkServerSocket
 * @brief   Encapsulate a socket that accepts connections.
 *
 *
 */

#ifndef vtkServerSocket_h
#define vtkServerSocket_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkSocket.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkClientSocket;
class VTKCOMMONSYSTEM_EXPORT vtkServerSocket : public vtkSocket
{
public:
  static vtkServerSocket* New();
  vtkTypeMacro(vtkServerSocket, vtkSocket);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Creates a server socket at a given port and binds given IPV4 address to it.
   * `bindAddr` defaults to INADDR_ANY (0.0.0.0) if not specified.
   * Returns -1 on error. 0 on success.
   */
  int CreateServer(int port, const std::string& bindAddr);
  int CreateServer(int port);
  ///@}

  /**
   * Waits for a connection. When a connection is received
   * a new vtkClientSocket object is created and returned.
   * Returns nullptr on timeout.
   */
  VTK_NEWINSTANCE vtkClientSocket* WaitForConnection(unsigned long msec = 0);

  /**
   * Returns the port on which the server is running.
   */
  int GetServerPort();

protected:
  vtkServerSocket();
  ~vtkServerSocket() override;

private:
  vtkServerSocket(const vtkServerSocket&) = delete;
  void operator=(const vtkServerSocket&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
