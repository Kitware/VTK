/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitPolyDataPiece.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitPolyDataPiece - Return specified piece, including specified
// number of ghost levels.
// .DESCRIPTION
// This filter updates the appropriate piece by requesting the piece from 
// process 0.  Process 0 always updates all of the data.  It is important that 
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitPolyDataPiece_h
#define __vtkTransmitPolyDataPiece_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkMultiProcessController.h"


class VTK_PARALLEL_EXPORT vtkTransmitPolyDataPiece : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkTransmitPolyDataPiece *New();
  vtkTypeRevisionMacro(vtkTransmitPolyDataPiece, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Turn on/off creating ghost cells (on by default).
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);
  
protected:
  vtkTransmitPolyDataPiece();
  ~vtkTransmitPolyDataPiece();
  vtkTransmitPolyDataPiece(const vtkTransmitPolyDataPiece&);
  void operator=(const vtkTransmitPolyDataPiece&);

  // Data generation method
  void Execute();
  void RootExecute();
  void SatelliteExecute(int procId);
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
 
  vtkPolyData *Buffer;
  int BufferPiece;
  int BufferNumberOfPieces;
  int BufferGhostLevel;

  int CreateGhostCells;
  vtkMultiProcessController *Controller;
};

#endif
