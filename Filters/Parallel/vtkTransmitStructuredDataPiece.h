/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitStructuredDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitRectilinearGridPiece - Redistributes data produced
// by serial readers
//
// .SECTION Description
// This filter can be used to redistribute data from producers that can't
// produce data in parallel. All data is produced on first process and
// the distributed to others using the multiprocess controller.


#ifndef vtkTransmitStructuredDataPiece_h
#define vtkTransmitStructuredDataPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitStructuredDataPiece : public vtkDataSetAlgorithm
{
public:
  static vtkTransmitStructuredDataPiece *New();
  vtkTypeMacro(vtkTransmitStructuredDataPiece, vtkDataSetAlgorithm);
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
  vtkTransmitStructuredDataPiece();
  ~vtkTransmitStructuredDataPiece();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkDataSet *input, vtkDataSet *output,
                   vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkDataSet *output,
                        vtkInformation *outInfo);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitStructuredDataPiece(const vtkTransmitStructuredDataPiece&); // Not implemented
  void operator=(const vtkTransmitStructuredDataPiece&); // Not implemented
};

#endif
