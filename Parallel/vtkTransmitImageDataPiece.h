/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitImageDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitImageDataPiece - For parallel processing, restrict IO to
// the first process in the cluste.r
//
// .SECTION Description
// This filter updates the appropriate piece by requesting the piece from 
// process 0.  Process 0 always updates all of the data.  It is important that 
// Execute get called on all processes, otherwise the filter will deadlock.


#ifndef __vtkTransmitImageDataPiece_h
#define __vtkTransmitImageDataPiece_h

#include "vtkImageAlgorithm.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkTransmitImageDataPiece : public vtkImageAlgorithm
{
public:
  static vtkTransmitImageDataPiece *New();
  vtkTypeMacro(vtkTransmitImageDataPiece, vtkImageAlgorithm);
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
  vtkTransmitImageDataPiece();
  ~vtkTransmitImageDataPiece();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkImageData *input, vtkImageData *output,
                   vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkImageData *output,
                        vtkInformation *outInfo);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
 
  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitImageDataPiece(const vtkTransmitImageDataPiece&); // Not implemented
  void operator=(const vtkTransmitImageDataPiece&); // Not implemented
};

#endif
