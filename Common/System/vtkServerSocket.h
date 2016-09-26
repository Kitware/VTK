/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServerSocket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkClientSocket;
class VTKCOMMONSYSTEM_EXPORT vtkServerSocket : public vtkSocket
{
public:
  static vtkServerSocket* New();
  vtkTypeMacro(vtkServerSocket, vtkSocket);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a server socket at a given port and binds to it.
   * Returns -1 on error. 0 on success.
   */
  int CreateServer(int port);

  /**
   * Waits for a connection. When a connection is received
   * a new vtkClientSocket object is created and returned.
   * Returns NULL on timeout.
   */
  vtkClientSocket* WaitForConnection(unsigned long msec=0);

  /**
   * Returns the port on which the server is running.
   */
  int GetServerPort();

protected:
  vtkServerSocket();
  ~vtkServerSocket() VTK_OVERRIDE;

private:
  vtkServerSocket(const vtkServerSocket&) VTK_DELETE_FUNCTION;
  void operator=(const vtkServerSocket&) VTK_DELETE_FUNCTION;
};


#endif

