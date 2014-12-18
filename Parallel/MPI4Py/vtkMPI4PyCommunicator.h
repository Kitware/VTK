/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPI4PyCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPI4PyCommunicator - Class for bridging MPI4Py with vtkMPICommunicator.

// .SECTION Description
// This class can be used to convert between VTK and MPI4Py communicators.

// .SECTION See Also
// vtkMPICommunicator

#ifndef vtkMPI4PyCommunicator_h
#define vtkMPI4PyCommunicator_h

#include "vtkPython.h" // For PyObject*; must be first

#include "vtkParallelMPI4PyModule.h" // For export macro
#include "vtkObject.h"

class vtkMPICommunicator;

class VTKPARALLELMPI4PY_EXPORT vtkMPI4PyCommunicator : public vtkObject
{
public:

  vtkTypeMacro(vtkMPI4PyCommunicator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMPI4PyCommunicator* New();
  vtkMPI4PyCommunicator();

  // Description:
  // Convert a VTK communicator into an mpi4py communicator.
  static PyObject* ConvertToPython(vtkMPICommunicator* comm);

  // Description:
  // Convert an mpi4py communicator into a VTK communicator.
  static vtkMPICommunicator* ConvertToVTK(PyObject* comm);

private:
  vtkMPI4PyCommunicator(const vtkMPI4PyCommunicator&);  // Not implemented.
  void operator=(const vtkMPI4PyCommunicator&);  // Not implemented.
};

#endif
