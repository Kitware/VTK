/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.h
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
// .NAME vtkPieceScalars - Sets all cell scalars from the update piece.
// .SECTION Description
// vtkPieceScalars is meant to display which piece is being requested
// as scalar values.  It is usefull for visualizing the partioning for
// streaming or distributed pipelines.
// .SECTION See Also
// vtkPolyDataStreamer

#ifndef __vtkPieceScalars_h
#define __vtkPieceScalars_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_PARALLEL_EXPORT vtkPieceScalars : public vtkDataSetToDataSetFilter
{
public:
  static vtkPieceScalars *New();

  vtkTypeRevisionMacro(vtkPieceScalars,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Option to centerate cell scalars of poi9nts scalars.  Default is point scalars.
  void SetScalarModeToCellData() {this->SetCellScalarsFlag(1);}
  void SetScalarModeToPointData() {this->SetCellScalarsFlag(0);}
  int GetScalarMode() {return this->CellScalarsFlag;}
  
  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are choosen between 0 and 1.  By default, random mode is off.
  vtkSetMacro(RandomMode, int);
  vtkGetMacro(RandomMode, int);
  vtkBooleanMacro(RandomMode, int);
  
protected:
  vtkPieceScalars();
  ~vtkPieceScalars();
  
  // Append the pieces.
  void Execute();
  
  vtkIntArray *MakePieceScalars(int piece, vtkIdType numScalars);
  vtkFloatArray *MakeRandomScalars(int piece, vtkIdType numScalars);
  
  vtkSetMacro(CellScalarsFlag,int);
  int CellScalarsFlag;
  int RandomMode;
private:
  vtkPieceScalars(const vtkPieceScalars&);  // Not implemented.
  void operator=(const vtkPieceScalars&);  // Not implemented.
};

#endif
