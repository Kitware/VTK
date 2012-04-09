/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitPolyDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitPolyDataPiece - Return specified piece, including specified
// number of ghost levels.
// .SECTION Description
// This filter updates the appropriate piece by requesting the piece from
// process 0.  Process 0 always updates all of the data.  It is important that
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitPolyDataPiece_h
#define __vtkTransmitPolyDataPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitPolyDataPiece : public vtkPolyDataAlgorithm
{
public:
  static vtkTransmitPolyDataPiece *New();
  vtkTypeMacro(vtkTransmitPolyDataPiece, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Turn on/off creating ghost cells (on by default).
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);

protected:
  vtkTransmitPolyDataPiece();
  ~vtkTransmitPolyDataPiece();

  // Data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkPolyData *input, vtkPolyData *output, vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkPolyData *output, vtkInformation *outInfo);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitPolyDataPiece(const vtkTransmitPolyDataPiece&); // Not implemented
  void operator=(const vtkTransmitPolyDataPiece&); // Not implemented
};

#endif
