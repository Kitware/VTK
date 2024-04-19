// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPainterCommunicator
 * ranks that will execute a painter chain.
 *
 *
 * A communicator that can safely be used inside a painter.
 * A simple container holding an MPI communicator. The simple API
 * is sufficient to allow serial code (no MPI available) to steer
 * execution.
 */

#ifndef vtkPPainterCommunicator_h
#define vtkPPainterCommunicator_h

#include "vtkPainterCommunicator.h"
#include "vtkRenderingParallelLICModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPPainterCommunicatorInternals;
class vtkMPICommunicatorOpaqueComm;

class VTKRENDERINGPARALLELLIC_EXPORT vtkPPainterCommunicator : public vtkPainterCommunicator
{
public:
  vtkPPainterCommunicator();
  ~vtkPPainterCommunicator() override;

  /**
   * Copier and assignment operators.
   */
  vtkPPainterCommunicator(const vtkPPainterCommunicator& other)
    : vtkPainterCommunicator(other)
  {
    this->Copy(&other, false);
  }

  vtkPPainterCommunicator& operator=(const vtkPPainterCommunicator& other)
  {
    this->Copy(&other, false);
    return *this;
  }

  /**
   * Copy the communicator.
   */
  void Copy(const vtkPainterCommunicator* other, bool ownership) override;

  /**
   * Duplicate the communicator.
   */
  void Duplicate(const vtkPainterCommunicator* other) override;

  ///@{
  /**
   * Query MPI for information about the communicator.
   */
  int GetRank() override;
  int GetSize() override;
  bool GetIsNull() override;
  ///@}

  ///@{
  /**
   * Query MPI for information about the world communicator.
   */
  int GetWorldRank() override;
  int GetWorldSize() override;
  ///@}

  /**
   * Query MPI state.
   */
  bool GetMPIInitialized() override { return vtkPPainterCommunicator::MPIInitialized(); }
  bool GetMPIFinalized() override { return vtkPPainterCommunicator::MPIFinalized(); }

  static bool MPIInitialized();
  static bool MPIFinalized();

  ///@{
  /**
   * Set/Get the communicator. Ownership is not assumed
   * thus caller must keep the commuicator alive while
   * this class is in use and free the communicator when
   * finished.
   */
  void SetCommunicator(vtkMPICommunicatorOpaqueComm* comm);
  void GetCommunicator(vtkMPICommunicatorOpaqueComm* comm);
  void* GetCommunicator();
  ///@}

  /**
   * Creates a new communicator with/without the calling processes
   * as indicated by the passed in flag, if not 0 the calling process
   * is included in the new communicator. The new communicator is
   * accessed via GetCommunicator. In parallel this call is mpi
   * collective on the world communicator. In serial this is a no-op.
   */
  void SubsetCommunicator(vtkMPICommunicatorOpaqueComm* comm, int include);

  /**
   * Get VTK's world communicator. Return's a null communictor if
   * MPI was not yet initialized.
   */
  static vtkMPICommunicatorOpaqueComm* GetGlobalCommunicator();

private:
  // PImpl for MPI datatypes
  vtkPPainterCommunicatorInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPPainterCommunicator.h
