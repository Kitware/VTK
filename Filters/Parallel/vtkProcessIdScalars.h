/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProcessIdScalars
 * @brief   Sets cell or point scalars to the processor rank.
 *
 *
 * vtkProcessIdScalars is meant to display which processor owns which cells
 * and points.  It is useful for visualizing the partitioning for
 * streaming or distributed pipelines.
 *
 * @sa
 * vtkPolyDataStreamer
*/

#ifndef vtkProcessIdScalars_h
#define vtkProcessIdScalars_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkFloatArray;
class vtkIntArray;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkProcessIdScalars : public vtkDataSetAlgorithm
{
public:
  static vtkProcessIdScalars *New();

  vtkTypeMacro(vtkProcessIdScalars,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Option to centerate cell scalars of points scalars.  Default is point
   * scalars.
   */
  void SetScalarModeToCellData() {this->SetCellScalarsFlag(1);}
  void SetScalarModeToPointData() {this->SetCellScalarsFlag(0);}
  int GetScalarMode() {return this->CellScalarsFlag;}

  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are chosen between 0 and 1.  By default, random
  // mode is off.
  vtkSetMacro(RandomMode, int);
  vtkGetMacro(RandomMode, int);
  vtkBooleanMacro(RandomMode, int);

  //@{
  /**
   * By defualt this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}


protected:
  vtkProcessIdScalars();
  ~vtkProcessIdScalars();

  // Append the pieces.
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkIntArray *MakeProcessIdScalars(int piece, vtkIdType numScalars);
  vtkFloatArray *MakeRandomScalars(int piece, vtkIdType numScalars);

  vtkSetMacro(CellScalarsFlag,int);
  int CellScalarsFlag;
  int RandomMode;

  vtkMultiProcessController* Controller;

private:
  vtkProcessIdScalars(const vtkProcessIdScalars&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProcessIdScalars&) VTK_DELETE_FUNCTION;
};

#endif
