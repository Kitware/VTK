// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractGeometry
 * @brief   extract cells that lie either entirely inside or outside of a specified
 *          implicit function
 *
 * vtkExtractGeometry extracts from its input dataset all cells that are either
 * completely inside or outside of a specified implicit function. Any type of
 * dataset can be input to this filter. On output the filter generates an
 * unstructured grid.
 *
 * To use this filter you must specify an implicit function. You must also
 * specify whether to extract cells laying inside or outside of the implicit
 * function. (The inside of an implicit function is the negative values
 * region.) An option exists to extract cells that are neither inside or
 * outside (i.e., boundary).
 *
 * A more efficient version of this filter is available for vtkPolyData input.
 * See vtkExtractPolyDataGeometry.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkExtractPolyDataGeometry vtkGeometryFilter vtkExtractVOI
 */

#ifndef vtkExtractGeometry_h
#define vtkExtractGeometry_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractGeometry : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractGeometry, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with ExtractInside turned on.
   */
  static vtkExtractGeometry* New();

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Boolean controls whether to extract cells that are inside of implicit
   * function (ExtractInside == 1) or outside of implicit function
   * (ExtractInside == 0).
   */
  vtkSetMacro(ExtractInside, vtkTypeBool);
  vtkGetMacro(ExtractInside, vtkTypeBool);
  vtkBooleanMacro(ExtractInside, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Boolean controls whether to extract cells that are partially inside.
   * By default, ExtractBoundaryCells is off.
   */
  vtkSetMacro(ExtractBoundaryCells, vtkTypeBool);
  vtkGetMacro(ExtractBoundaryCells, vtkTypeBool);
  vtkBooleanMacro(ExtractBoundaryCells, vtkTypeBool);
  vtkSetMacro(ExtractOnlyBoundaryCells, vtkTypeBool);
  vtkGetMacro(ExtractOnlyBoundaryCells, vtkTypeBool);
  vtkBooleanMacro(ExtractOnlyBoundaryCells, vtkTypeBool);
  ///@}

protected:
  vtkExtractGeometry(vtkImplicitFunction* f = nullptr);
  ~vtkExtractGeometry() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkImplicitFunction* ImplicitFunction;
  vtkTypeBool ExtractInside;
  vtkTypeBool ExtractBoundaryCells;
  vtkTypeBool ExtractOnlyBoundaryCells;

private:
  vtkExtractGeometry(const vtkExtractGeometry&) = delete;
  void operator=(const vtkExtractGeometry&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
