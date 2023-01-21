/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanUnstructuredGrid.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCleanUnstructuredGrid
 * @brief   merge duplicate points
 *
 *
 * vtkCleanUnstructuredGrid is a filter that takes unstructured grid data as
 * input and generates unstructured grid data as output. vtkCleanUnstructuredGrid can
 * merge duplicate points (with coincident coordinates) using the vtkMergePoints object
 * to merge points.
 *
 * @sa
 * vtkCleanPolyData
 */

#ifndef vtkCleanUnstructuredGrid_h
#define vtkCleanUnstructuredGrid_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;
class vtkDataSet;

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkCleanUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCleanUnstructuredGrid* New();

  vtkTypeMacro(vtkCleanUnstructuredGrid, vtkUnstructuredGridAlgorithm);

  // By default ToleranceIsAbsolute is false and Tolerance is
  // a fraction of Bounding box diagonal, if true, AbsoluteTolerance is
  // used when adding points to locator (merging)
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);

  // Specify tolerance in terms of fraction of bounding box length.
  // Default is 0.0.
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);

  // Specify tolerance in absolute terms. Default is 1.0.
  vtkSetClampMacro(AbsoluteTolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(AbsoluteTolerance, double);

  //@{
  /**
   * Set/Get a spatial locator for speeding the search process. By
   * default an instance of vtkMergePoints is used.
   */
  virtual void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  //@}

  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator(vtkDataSet* input = nullptr);

  // Release locator
  void ReleaseLocator() { this->SetLocator(nullptr); }

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCleanUnstructuredGrid() = default;
  ~vtkCleanUnstructuredGrid() override;

  // options for managing point merging tolerance
  bool ToleranceIsAbsolute = false;
  double Tolerance = 0.0;
  double AbsoluteTolerance = 1.0;
  vtkIncrementalPointLocator* Locator = nullptr;
  int OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&) = delete;
  void operator=(const vtkCleanUnstructuredGrid&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
