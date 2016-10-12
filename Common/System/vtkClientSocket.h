/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientSocket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClientSocket
 * @brief   Encapsulates a client socket.
*/

#ifndef vtkClientSocket_h
#define vtkClientSocket_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkSocket.h"
class vtkServerSocket;

class VTKCOMMONSYSTEM_EXPORT vtkClientSocket : public vtkSocket
{
public:
  static vtkClientSocket* New();
  vtkTypeMacro(vtkClientSocket, vtkSocket);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Connects to host. Returns 0 on success, -1 on error.
   */
  int ConnectToServer(const char* hostname, int port);

  //@{
  /**
   * Returns if the socket is on the connecting side (the side that requests a
   * ConnectToServer() or on the connected side (the side that was waiting for
   * the client to connect). This is used to disambiguate the two ends of a socket
   * connection.
   */
  vtkGetMacro(ConnectingSide, bool);
  //@}

protected:
  vtkClientSocket();
  ~vtkClientSocket() VTK_OVERRIDE;

  vtkSetMacro(ConnectingSide, bool);
  bool ConnectingSide;
  friend class vtkServerSocket;
private:
  vtkClientSocket(const vtkClientSocket&) VTK_DELETE_FUNCTION;
  void operator=(const vtkClientSocket&) VTK_DELETE_FUNCTION;

};


#endif

