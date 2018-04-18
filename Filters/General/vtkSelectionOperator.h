/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionOperator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectionOperator.h
 * @brief   Computes the portion of a dataset which is inside a selection
 *
 */

#ifndef vtkSelectionOperator_h
#define vtkSelectionOperator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkObject.h"

class vtkDataSet;
class vtkSelectionNode;
class vtkSignedCharArray;
class vtkTable;

class VTKFILTERSGENERAL_EXPORT vtkSelectionOperator : public vtkObject
{
  public:
  vtkTypeMacro(vtkSelectionOperator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Initialize(vtkSelectionNode* node) = 0;

  virtual void ComputePointsInside(vtkDataSet* input, vtkSignedCharArray* pointsInside);
  virtual void ComputeCellsInside(vtkDataSet* input, vtkSignedCharArray* cellsInside);
  virtual void ComputeRowsInside(vtkTable* input, vtkSignedCharArray* rowsInside);
protected:
  vtkSelectionOperator();
  virtual ~vtkSelectionOperator() override;
private:
  vtkSelectionOperator(const vtkSelectionOperator&) = delete;
  void operator=(const vtkSelectionOperator&) = delete;
};

#endif
