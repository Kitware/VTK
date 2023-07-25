// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericInterpolatedVelocityField
 * @brief   Interface for obtaining
 * interpolated velocity values
 *
 * vtkGenericInterpolatedVelocityField acts as a continuous velocity field
 * by performing cell interpolation on the underlying vtkDataSet.
 * This is a concrete sub-class of vtkFunctionSet with
 * NumberOfIndependentVariables = 4 (x,y,z,t) and
 * NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
 * is performed, the cell which contains the point (x,y,z) has to
 * be found by calling FindCell. This is a computationally expansive
 * operation. In certain cases, the cell search can be avoided or shortened
 * by providing a guess for the cell iterator. For example, in streamline
 * integration, the next evaluation is usually in the same or a neighbour
 * cell. For this reason, vtkGenericInterpolatedVelocityField stores the last
 * cell iterator. If caching is turned on, it uses this iterator as the
 * starting point.
 *
 * @warning
 * vtkGenericInterpolatedVelocityField is not thread safe. A new instance
 * should be created by each thread.
 *
 * @sa
 * vtkFunctionSet vtkGenericStreamTracer
 */

#ifndef vtkGenericInterpolatedVelocityField_h
#define vtkGenericInterpolatedVelocityField_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkFunctionSet.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericDataSet;
class vtkGenericCellIterator;
class vtkGenericAdaptorCell;

class vtkGenericInterpolatedVelocityFieldDataSetsType;

class VTKCOMMONDATAMODEL_EXPORT vtkGenericInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkGenericInterpolatedVelocityField, vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a vtkGenericInterpolatedVelocityField with no initial data set.
   * Caching is on. LastCellId is set to -1.
   */
  static vtkGenericInterpolatedVelocityField* New();

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field, f, at (x, y, z, t).
   * For now, t is ignored.
   */
  int FunctionValues(double* x, double* f) override;

  /**
   * Add a dataset used for the implicit function evaluation.
   * If more than one dataset is added, the evaluation point is
   * searched in all until a match is found. THIS FUNCTION
   * DOES NOT CHANGE THE REFERENCE COUNT OF dataset FOR THREAD
   * SAFETY REASONS.
   */
  virtual void AddDataSet(vtkGenericDataSet* dataset);

  /**
   * Set the last cell id to -1 so that the next search does not
   * start from the previous cell
   */
  void ClearLastCell();

  /**
   * Return the cell cached from last evaluation.
   */
  vtkGenericAdaptorCell* GetLastCell();

  /**
   * Returns the interpolation weights cached from last evaluation
   * if the cached cell is valid (returns 1). Otherwise, it does not
   * change w and returns 0.
   */
  int GetLastLocalCoordinates(double pcoords[3]);

  ///@{
  /**
   * Turn caching on/off.
   */
  vtkGetMacro(Caching, vtkTypeBool);
  vtkSetMacro(Caching, vtkTypeBool);
  vtkBooleanMacro(Caching, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Caching statistics.
   */
  vtkGetMacro(CacheHit, int);
  vtkGetMacro(CacheMiss, int);
  ///@}

  ///@{
  /**
   * If you want to work with an arbitrary vector array, then set its name
   * here. By default this in nullptr and the filter will use the active vector
   * array.
   */
  vtkGetStringMacro(VectorsSelection);
  void SelectVectors(const char* fieldName) { this->SetVectorsSelection(fieldName); }
  ///@}

  ///@{
  /**
   * Returns the last dataset that was visited. Can be used
   * as a first guess as to where the next point will be as
   * well as to avoid searching through all datasets to get
   * more information about the point.
   */
  vtkGetObjectMacro(LastDataSet, vtkGenericDataSet);
  ///@}

  /**
   * Copy the user set parameters from source. This copies
   * the Caching parameters. Sub-classes can add more after
   * chaining.
   */
  virtual void CopyParameters(vtkGenericInterpolatedVelocityField* from);

protected:
  vtkGenericInterpolatedVelocityField();
  ~vtkGenericInterpolatedVelocityField() override;

  vtkGenericCellIterator* GenCell; // last cell

  double LastPCoords[3]; // last local coordinates
  int CacheHit;
  int CacheMiss;
  vtkTypeBool Caching;

  vtkGenericDataSet* LastDataSet;

  vtkSetStringMacro(VectorsSelection);
  char* VectorsSelection;

  vtkGenericInterpolatedVelocityFieldDataSetsType* DataSets;

  int FunctionValues(vtkGenericDataSet* dataset, double* x, double* f);

  static const double TOLERANCE_SCALE;

private:
  vtkGenericInterpolatedVelocityField(const vtkGenericInterpolatedVelocityField&) = delete;
  void operator=(const vtkGenericInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
