/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterCommunicator.h

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
// A simple container holding a handle to an MPI communicator.
// This API is sufficient to allow for control flow with/without
// MPI. The parallel parts of the code should use the derived
// class vtkPPainterCommunicator.
#ifndef __vtkPainterCommunicator_h
#define __vtkPainterCommunicator_h

#include "vtkRenderingLICModule.h" // for export macro

class VTKRENDERINGLIC_EXPORT vtkPainterCommunicator
{
public:
  vtkPainterCommunicator(){}
  virtual ~vtkPainterCommunicator(){}

  // Description:
  // Copy and assignment operators. Both use Copy internally
  // and do take ownership.
  vtkPainterCommunicator(const vtkPainterCommunicator &other)
    { this->Copy(&other, false); }

  vtkPainterCommunicator &operator=(const vtkPainterCommunicator &other)
    { this->Copy(&other, false); return *this; }

  // Description:
  // Copy the communicator, the flag indicates if ownership
  // should be assumed. The owner is responsible for free'ing
  // the communicator.
  virtual void Copy(const vtkPainterCommunicator *, bool){}

  // Description:
  // Duplicate the communicator.
  virtual void Duplicate(const vtkPainterCommunicator *){}

  // Description:
  // Querry MPI about the communicator.
  virtual int GetRank(){ return 0; }
  virtual int GetSize(){ return 1; }
  virtual bool GetIsNull(){ return false; }

  // Description:
  // Querry MPI about the world communicator.
  virtual int GetWorldRank(){ return 0; }
  virtual int GetWorldSize(){ return 1; }

  // Description:
  // Querry MPI about its state.
  virtual bool GetMPIInitialized(){ return false; }
  virtual bool GetMPIFinalized(){ return true; }
};

#endif
// VTK-HeaderTest-Exclude: vtkPainterCommunicator.h
