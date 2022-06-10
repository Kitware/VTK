/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeQuartiles.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkComputeQuartiles
 * @brief   Extract quartiles and extremum values
 * of all columns of a table or all fields of a dataset.
 *
 *
 * vtkComputeQuartiles accepts any vtkDataObject as input and produces a
 * vtkTable data as output.
 * This filter can be used to generate a table to create box plots
 * using a vtkPlotBox instance.
 * The filter internally uses vtkOrderStatistics to compute quartiles.
 *
 * Note: This class is being kept for backwards compatibility. Please use vtkComputeQuantiles
 * instead which is the generalized version of this filter.
 *
 * @sa
 * vtkTableAlgorithm vtkOrderStatistics vtkPlotBox vtkChartBox
 *
 * @par Thanks:
 * This class was written by Kitware SAS and supported by EDF - www.edf.fr
 */

#ifndef vtkComputeQuartiles_h
#define vtkComputeQuartiles_h

#include "vtkComputeQuantiles.h"
#include "vtkFiltersStatisticsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSTATISTICS_EXPORT vtkComputeQuartiles : public vtkComputeQuantiles
{
public:
  static vtkComputeQuartiles* New();
  vtkTypeMacro(vtkComputeQuartiles, vtkComputeQuantiles);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkComputeQuartiles();
  ~vtkComputeQuartiles() override = default;

private:
  void operator=(const vtkComputeQuartiles&) = delete;
  vtkComputeQuartiles(const vtkComputeQuartiles&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
