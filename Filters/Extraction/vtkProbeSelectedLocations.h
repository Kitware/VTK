// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProbeSelectedLocations
 * @brief   similar to vtkExtractSelection
 * except that it interpolates the point attributes at the probe locations.
 *
 * vtkProbeSelectedLocations is similar to vtkExtractSelection except
 * that it interpolates the point attributes at the probe location. This is
 * equivalent to the vtkProbeFilter except that the probe locations are provided
 * by a vtkSelection. The FieldType of the input vtkSelection is immaterial and
 * is ignored. The ContentType of the input vtkSelection must be
 * vtkSelection::LOCATIONS.
 */

#ifndef vtkProbeSelectedLocations_h
#define vtkProbeSelectedLocations_h

#include "vtkExtractSelectionBase.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSEXTRACTION_EXPORT vtkProbeSelectedLocations : public vtkExtractSelectionBase
{
public:
  static vtkProbeSelectedLocations* New();
  vtkTypeMacro(vtkProbeSelectedLocations, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkProbeSelectedLocations();
  ~vtkProbeSelectedLocations() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkProbeSelectedLocations(const vtkProbeSelectedLocations&) = delete;
  void operator=(const vtkProbeSelectedLocations&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
