/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitUnstructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitUnstructuredGridPiece - Return specified piece, including specified
// number of ghost levels.
// .SECTION Description
// This filter updates the appropriate piece by requesting the piece from
// process 0.  Process 0 always updates all of the data.  It is important that
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitUnstructuredGridPiece_h
#define __vtkTransmitUnstructuredGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitUnstructuredGridPiece : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkTransmitUnstructuredGridPiece *New();
  vtkTypeMacro(vtkTransmitUnstructuredGridPiece, vtkUnstructuredGridAlgorithm);
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
  vtkTransmitUnstructuredGridPiece();
  ~vtkTransmitUnstructuredGridPiece();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkUnstructuredGrid *input, vtkUnstructuredGrid *output,
                   vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkUnstructuredGrid *output,
                        vtkInformation *outInfo);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitUnstructuredGridPiece(const vtkTransmitUnstructuredGridPiece&); // Not implemented
  void operator=(const vtkTransmitUnstructuredGridPiece&); // Not implemented
};

#endif
