/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPieceScalars
 * @brief   Sets all cell scalars from the update piece.
 *
 *
 * vtkPieceScalars is meant to display which piece is being requested
 * as scalar values.  It is useful for visualizing the partitioning for
 * streaming or distributed pipelines.
 *
 * @sa
 * vtkPolyDataStreamer
*/

#ifndef vtkPieceScalars_h
#define vtkPieceScalars_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkFloatArray;
class vtkIntArray;

class VTKFILTERSPARALLEL_EXPORT vtkPieceScalars : public vtkDataSetAlgorithm
{
public:
  static vtkPieceScalars *New();

  vtkTypeMacro(vtkPieceScalars,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Option to centerate cell scalars of points scalars.  Default is point scalars.
   */
  void SetScalarModeToCellData() {this->SetCellScalarsFlag(1);}
  void SetScalarModeToPointData() {this->SetCellScalarsFlag(0);}
  int GetScalarMode() {return this->CellScalarsFlag;}

  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are chosen between 0 and 1.  By default, random mode is off.
  vtkSetMacro(RandomMode, int);
  vtkGetMacro(RandomMode, int);
  vtkBooleanMacro(RandomMode, int);

protected:
  vtkPieceScalars();
  ~vtkPieceScalars();

  // Append the pieces.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkIntArray *MakePieceScalars(int piece, vtkIdType numScalars);
  vtkFloatArray *MakeRandomScalars(int piece, vtkIdType numScalars);

  vtkSetMacro(CellScalarsFlag,int);
  int CellScalarsFlag;
  int RandomMode;
private:
  vtkPieceScalars(const vtkPieceScalars&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPieceScalars&) VTK_DELETE_FUNCTION;
};

#endif
