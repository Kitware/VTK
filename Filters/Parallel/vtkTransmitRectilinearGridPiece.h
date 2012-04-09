/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitRectilinearGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitRectilinearGridPiece - For parallel processing, restrict 
// IO to the first process in the cluster.
//
// .SECTION Description
// This filter updates the appropriate piece by requesting the piece from 
// process 0.  Process 0 always updates all of the data.  It is important that 
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitRectilinearGridPiece_h
#define __vtkTransmitRectilinearGridPiece_h

#include "vtkRectilinearGridAlgorithm.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkTransmitRectilinearGridPiece : public vtkRectilinearGridAlgorithm
{
public:
  static vtkTransmitRectilinearGridPiece *New();
  vtkTypeMacro(vtkTransmitRectilinearGridPiece, vtkRectilinearGridAlgorithm);
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
  vtkTransmitRectilinearGridPiece();
  ~vtkTransmitRectilinearGridPiece();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkRectilinearGrid *input, vtkRectilinearGrid *output,
                   vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkRectilinearGrid *output,
                        vtkInformation *outInfo);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
 
  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitRectilinearGridPiece(const vtkTransmitRectilinearGridPiece&); // Not implemented
  void operator=(const vtkTransmitRectilinearGridPiece&); // Not implemented
};

#endif
