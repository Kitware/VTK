/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceRequestFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPieceRequestFilter - Sets the piece request for upstream filters.
// .SECTION Description
// Sends the piece and number of pieces to upstream filters; passes the input
// to the output unmodified.

#ifndef vtkPieceRequestFilter_h
#define vtkPieceRequestFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataObject;

class VTKFILTERSPARALLEL_EXPORT vtkPieceRequestFilter : public vtkAlgorithm
{
public:
  static vtkPieceRequestFilter *New();
  vtkTypeMacro(vtkPieceRequestFilter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The total number of pieces.
  vtkSetClampMacro(NumberOfPieces, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // The piece to extract.
  vtkSetClampMacro(Piece, int, 0, VTK_INT_MAX);
  vtkGetMacro(Piece, int);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);

  // Description:
  // Set an input of this algorithm.
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkPieceRequestFilter();
  ~vtkPieceRequestFilter() {}

  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int NumberOfPieces;
  int Piece;

private:
  vtkPieceRequestFilter(const vtkPieceRequestFilter&);  // Not implemented.
  void operator=(const vtkPieceRequestFilter&);  // Not implemented.
};

#endif


