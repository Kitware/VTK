/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitUnstructuredGridPiece.h
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
// .NAME vtkTransmitUnstructuredGridPiece - Return specified piece, including specified
// number of ghost levels.
// .DESCRIPTION
// This filter updates the appropriate piece by requesting the piece from 
// process 0.  Process 0 always updates all of the data.  It is important that 
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitUnstructuredGridPiece_h
#define __vtkTransmitUnstructuredGridPiece_h

#include "vtkUnstructuredGridToUnstructuredGridFilter.h"
#include "vtkMultiProcessController.h"


class VTK_PARALLEL_EXPORT vtkTransmitUnstructuredGridPiece : public vtkUnstructuredGridToUnstructuredGridFilter
{
public:
  static vtkTransmitUnstructuredGridPiece *New();
  vtkTypeRevisionMacro(vtkTransmitUnstructuredGridPiece, vtkUnstructuredGridToUnstructuredGridFilter);
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
  vtkTransmitUnstructuredGridPiece();
  ~vtkTransmitUnstructuredGridPiece();
  vtkTransmitUnstructuredGridPiece(const vtkTransmitUnstructuredGridPiece&);
  void operator=(const vtkTransmitUnstructuredGridPiece&);

  // Data generation method
  void Execute();
  void RootExecute();
  void SatelliteExecute(int procId);
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
 
  int CreateGhostCells;
  vtkMultiProcessController *Controller;
};

#endif
