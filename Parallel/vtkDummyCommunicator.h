/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDummyCommunicator - Dummy controller for single process applications.
//
// .SECTION Description
//
// This is a dummy communicator, which can be used by applications that always
// require a controller but are also compiled on systems without threads or MPI.
// Because there is always only one process, no real communication takes place.
//

#ifndef __vtkDummyCommunicator_h
#define __vtkDummyCommunicator_h

#include "vtkCommunicator.h"

class VTK_PARALLEL_EXPORT vtkDummyCommunicator : public vtkCommunicator
{
public:
  vtkTypeMacro(vtkDummyCommunicator, vtkCommunicator);
  static vtkDummyCommunicator *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Since there is no one to communicate with, these methods just report an
  // error.
  virtual int SendVoidArray(const void *, vtkIdType, int, int, int) {
    vtkWarningMacro("There is no one to send to.");
    return 0;
  }
  virtual int ReceiveVoidArray(void *, vtkIdType, int, int, int) {
    vtkWarningMacro("There is no one to receive from.");
    return 0;
  }

protected:
  vtkDummyCommunicator();
  virtual ~vtkDummyCommunicator();

private:
  vtkDummyCommunicator(const vtkDummyCommunicator &);   // Not implemented
  void operator=(const vtkDummyCommunicator &);         // Not implemented
};

#endif //__vtkDummyCommunicator_h
