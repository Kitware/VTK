// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkmExtractVOI
 * @brief   select piece (e.g., volume of interest) and/or subsample structured points dataset
 *
 * vtkmExtractVOI is a filter that selects a portion of an input structured
 * points dataset, or subsamples an input dataset. (The selected portion of
 * interested is referred to as the Volume Of Interest, or VOI.) The output of
 * this filter is a structured points dataset. The filter treats input data
 * of any topological dimension (i.e., point, line, image, or volume) and can
 * generate output data of any topological dimension.
 *
 * To use this filter set the VOI ivar which are i-j-k min/max indices that
 * specify a rectangular region in the data. (Note that these are 0-offset.)
 * You can also specify a sampling rate to subsample the data.
 *
 * Typical applications of this filter are to extract a slice from a volume
 * for image processing, subsampling large volumes to reduce data size, or
 * extracting regions of a volume with interesting data.
 *
 */
#ifndef vtkmExtractVOI_h
#define vtkmExtractVOI_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // for export macro
#include "vtkExtractVOI.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmExtractVOI : public vtkExtractVOI
{
public:
  vtkTypeMacro(vtkmExtractVOI, vtkExtractVOI);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmExtractVOI* New();

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if Viskores fails to run. When the flag is on,
   * the filter will generate an error if Viskores fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  vtkmExtractVOI();
  ~vtkmExtractVOI() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;

private:
  vtkmExtractVOI(const vtkmExtractVOI&) = delete;
  void operator=(const vtkmExtractVOI&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmExtractVOI_h
