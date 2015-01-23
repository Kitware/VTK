/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPainterCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPainterCommunicator -- A communicator containing only
// ranks that will execute a painter chain.
//
// .SECTION Description
// A communicator that can safely be used inside a painter.
// A simple container holding an MPI communicator. The simple API
// is sufficient to allow serial code (no MPI available) to steer
// execution.
#ifndef vtkPPainterCommunicator_h
#define vtkPPainterCommunicator_h

#include "vtkPainterCommunicator.h"
#include "vtkRenderingParallelLICModule.h" // for export macro

class vtkPPainterCommunicatorInternals;
class vtkMPICommunicatorOpaqueComm;

class VTKRENDERINGPARALLELLIC_EXPORT vtkPPainterCommunicator : public vtkPainterCommunicator
{
public:
  vtkPPainterCommunicator();
  virtual ~vtkPPainterCommunicator();

  // Description:
  // Copier and assignment operators.
  vtkPPainterCommunicator(const vtkPPainterCommunicator &other) : vtkPainterCommunicator(other)
    { this->Copy(&other, false); }

  vtkPPainterCommunicator &operator=(const vtkPPainterCommunicator &other)
    { this->Copy(&other, false); return *this; }

  // Description:
  // Copy the communicator.
  virtual void Copy(const vtkPainterCommunicator *other, bool ownership);

  // Description:
  // Duplicate the communicator.
  virtual void Duplicate(const vtkPainterCommunicator *other);

  // Description:
  // Querry MPI for inforrmation about the communicator.
  virtual int GetRank();
  virtual int GetSize();
  virtual bool GetIsNull();

  // Description:
  // Querry MPI for information a bout the world communicator.
  virtual int GetWorldRank();
  virtual int GetWorldSize();

  // Description:
  // Querry MPI state.
  virtual bool GetMPIInitialized(){ return this->MPIInitialized(); }
  virtual bool GetMPIFinalized(){ return this->MPIFinalized(); }

  static bool MPIInitialized();
  static bool MPIFinalized();

  // Description:
  // Set/Get the communicator. Ownership is not assumed
  // thus caller must keep the commuicator alive while
  // this class is in use and free the communicator when
  // finished.
  void SetCommunicator(vtkMPICommunicatorOpaqueComm *comm);
  void GetCommunicator(vtkMPICommunicatorOpaqueComm *comm);
  void *GetCommunicator();

  // Description:
  // Creates a new communicator with/without the calling processes
  // as indicated by the passed in flag, if not 0 the calling process
  // is included in the new communicator. The new communicator is
  // accessed via GetCommunicator. In parallel this call is mpi
  // collective on the world communicator. In serial this is a no-op.
  void SubsetCommunicator(vtkMPICommunicatorOpaqueComm *comm, int include);

  // Description:
  // Get VTK's world communicator. Return's a null communictor if
  // MPI was not yet initialized.
  static vtkMPICommunicatorOpaqueComm *GetGlobalCommunicator();

private:
  // PImpl for MPI datatypes
  vtkPPainterCommunicatorInternals *Internals;
};

#endif
// VTK-HeaderTest-Exclude: vtkPPainterCommunicator.h
