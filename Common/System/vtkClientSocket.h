// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClientSocket
 * @brief   Encapsulates a client socket.
 */

#ifndef vtkClientSocket_h
#define vtkClientSocket_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkSocket.h"
VTK_ABI_NAMESPACE_BEGIN
class vtkServerSocket;

class VTKCOMMONSYSTEM_EXPORT vtkClientSocket : public vtkSocket
{
public:
  static vtkClientSocket* New();
  vtkTypeMacro(vtkClientSocket, vtkSocket);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Connects to host. Returns 0 on success, -1 on error.
   */
  int ConnectToServer(const char* hostname, int port);

  ///@{
  /**
   * Returns if the socket is on the connecting side (the side that requests a
   * ConnectToServer() or on the connected side (the side that was waiting for
   * the client to connect). This is used to disambiguate the two ends of a socket
   * connection.
   */
  vtkGetMacro(ConnectingSide, bool);
  ///@}

protected:
  vtkClientSocket();
  ~vtkClientSocket() override;

  vtkSetMacro(ConnectingSide, bool);
  bool ConnectingSide;
  friend class vtkServerSocket;

private:
  vtkClientSocket(const vtkClientSocket&) = delete;
  void operator=(const vtkClientSocket&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
