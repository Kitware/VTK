/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractFunctionalBagPlot.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractFunctionalBagPlot
 *
 *
 * From an input table containing series on port 0 and another table
 * describing densities on port 1 (for instance obtained by applying
 * filter  vtkHighestDensityRegionsStatistics, this filter generates
 * a table containing all the columns of the input port 0 plus two 2
 * components columns containing the bag series to be used by
 * vtkFunctionalBagPlot.
 *
 * @sa
 * vtkFunctionalBagPlot vtkHighestDensityRegionsStatistics
*/

#ifndef vtkExtractFunctionalBagPlot_h
#define vtkExtractFunctionalBagPlot_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"


class VTKFILTERSSTATISTICS_EXPORT vtkExtractFunctionalBagPlot : public vtkTableAlgorithm
{
public:
  static vtkExtractFunctionalBagPlot* New();
  vtkTypeMacro(vtkExtractFunctionalBagPlot, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Density value for the median quartile.
  vtkSetMacro(DensityForP50, double);

  //@{
  /**
   * Density value for the user defined quartile.
   */
  vtkSetMacro(DensityForPUser, double);
  vtkSetMacro(PUser, int);
  //@}

protected:
  vtkExtractFunctionalBagPlot();
  ~vtkExtractFunctionalBagPlot() override;

  int RequestData(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  char *P50String;
  char *PUserString;
  double DensityForP50;
  double DensityForPUser;
  int PUser;

private:
  vtkExtractFunctionalBagPlot( const vtkExtractFunctionalBagPlot& ) = delete;
  void operator = ( const vtkExtractFunctionalBagPlot& ) = delete;
};

#endif // vtkExtractFunctionalBagPlot_h
