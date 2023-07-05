// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRInterpolatedVelocityField
 * @brief   A concrete class for obtaining
 *  the interpolated velocity values at a point in AMR data.
 *
 * The main functionality supported here is the point location inside
 * vtkOverlappingAMR data set.
 */

#ifndef vtkAMRInterpolatedVelocityField_h
#define vtkAMRInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro

#include "vtkAbstractInterpolatedVelocityField.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOverlappingAMR;

class VTKFILTERSFLOWPATHS_EXPORT vtkAMRInterpolatedVelocityField
  : public vtkAbstractInterpolatedVelocityField
{
public:
  ///@{
  /**
   * Standard methods for obtaining type information and printing the object state.
   */
  static vtkAMRInterpolatedVelocityField* New();
  vtkTypeMacro(vtkAMRInterpolatedVelocityField, vtkAbstractInterpolatedVelocityField);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   *
   * Specify the AMR dataset to process.
   */
  virtual void SetAmrDataSet(vtkOverlappingAMR*);
  vtkGetObjectMacro(AmrDataSet, vtkOverlappingAMR);
  void SetAMRData(vtkOverlappingAMR* amr) { this->SetAmrDataSet(amr); }
  ///@}

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * interpolators between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  void CopyParameters(vtkAbstractInterpolatedVelocityField* from) override;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId(vtkIdType c) override { this->Superclass::SetLastCellId(c); }

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point p.
   * If it succeeds, then both the last data set (this->LastDataSet) and
   * the last data set location (this->LastLevel, this->LastId) will be
   * set according to where p is found.  If it fails, either p is out of
   * bound, in which case both the last data set and the last location
   * will be invalid or, in a multi-process setting, p is inbound but not
   * on the processor.  In the last case, the last data set location is
   * still valid
   */
  int FunctionValues(double* x, double* f) override;

  /**
   * Helper function to locator the grid within an AMR dataset.
   */
  static bool FindGrid(
    double q[3], vtkOverlappingAMR* amrds, unsigned int& level, unsigned int& gridId);

  ///@{
  /**
   * Methods to support local caching while searching for AMR datasets.
   */
  bool GetLastDataSetLocation(unsigned int& level, unsigned int& id);
  bool SetLastDataSet(int level, int id);
  void SetLastCellId(vtkIdType c, int dataindex) override;
  ///@}

protected:
  vtkOverlappingAMR* AmrDataSet;
  int LastLevel;
  int LastId;

  vtkAMRInterpolatedVelocityField();
  ~vtkAMRInterpolatedVelocityField() override;
  int FunctionValues(vtkDataSet* ds, double* x, double* f) override
  {
    return this->Superclass::FunctionValues(ds, x, f);
  }

  /**
   * Method to initialize the velocity field. Generally this must be called when
   * performing threaded operations; however if not called apriori it will be called in
   * the first call to FunctionValues(), which implicitly assumes that this is being
   * used in serial operation.
   */
  int SelfInitialize() override;

private:
  vtkAMRInterpolatedVelocityField(const vtkAMRInterpolatedVelocityField&) = delete;
  void operator=(const vtkAMRInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
