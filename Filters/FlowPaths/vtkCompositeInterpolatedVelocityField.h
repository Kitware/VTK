/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeInterpolatedVelocityField
 * @brief   An abstract class for
 *  obtaining the interpolated velocity values at a point
 *
 *  vtkCompositeInterpolatedVelocityField acts as a continuous velocity field
 *  by performing cell interpolation on one or more underlying vtkDataSets.
 *
 * @warning
 *  vtkCompositeInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  vtkInterpolatedVelocityField vtkCellLocatorInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
 *  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamTracer
 */

#ifndef vtkCompositeInterpolatedVelocityField_h
#define vtkCompositeInterpolatedVelocityField_h

#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkFiltersFlowPathsModule.h" // For export macro

#include <vector> // STL Header; Required for vector

class vtkDataSet;
class vtkDataArray;
class vtkPointData;
class vtkGenericCell;
class vtkCompositeInterpolatedVelocityFieldDataSetsType;

class VTKFILTERSFLOWPATHS_EXPORT vtkCompositeInterpolatedVelocityField
  : public vtkAbstractInterpolatedVelocityField
{
public:
  vtkTypeMacro(vtkCompositeInterpolatedVelocityField, vtkAbstractInterpolatedVelocityField);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a dataset for implicit velocity function evaluation. If more than
   * one dataset is added, the evaluation point is searched in all until a
   * match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
   * dataset FOR THREAD SAFETY REASONS.
   */
  virtual void AddDataSet(vtkDataSet* dataset) = 0;

  //@{
  /**
   * Get the most recently visited dataset and its id. The dataset is used
   * for a guess regarding where the next point will be, without searching
   * through all datasets. When setting the last dataset, care is needed as
   * no reference counting or checks are performed. This feature is intended
   * for custom interpolators only that cache datasets independently.
   */
  vtkGetMacro(LastDataSetIndex, int);
  //@}

protected:
  vtkCompositeInterpolatedVelocityField();
  ~vtkCompositeInterpolatedVelocityField() override;

  int LastDataSetIndex;
  vtkCompositeInterpolatedVelocityFieldDataSetsType* DataSets;

private:
  vtkCompositeInterpolatedVelocityField(const vtkCompositeInterpolatedVelocityField&) = delete;
  void operator=(const vtkCompositeInterpolatedVelocityField&) = delete;
};

typedef std::vector<vtkDataSet*> DataSetsTypeBase;
class vtkCompositeInterpolatedVelocityFieldDataSetsType : public DataSetsTypeBase
{
};

#endif
