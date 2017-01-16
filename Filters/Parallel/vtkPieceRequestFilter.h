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
/**
 * @class   vtkPieceRequestFilter
 * @brief   Sets the piece request for upstream filters.
 *
 * Sends the piece and number of pieces to upstream filters; passes the input
 * to the output unmodified.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The total number of pieces.
   */
  vtkSetClampMacro(NumberOfPieces, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * The piece to extract.
   */
  vtkSetClampMacro(Piece, int, 0, VTK_INT_MAX);
  vtkGetMacro(Piece, int);
  //@}

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  //@}

  //@{
  /**
   * Set an input of this algorithm.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector) VTK_OVERRIDE;

protected:
  vtkPieceRequestFilter();
  ~vtkPieceRequestFilter() VTK_OVERRIDE {}

  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int NumberOfPieces;
  int Piece;

private:
  vtkPieceRequestFilter(const vtkPieceRequestFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPieceRequestFilter&) VTK_DELETE_FUNCTION;
};

#endif


