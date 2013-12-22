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
// .NAME vtkExtractFunctionalBagPlot
//
// .SECTION Description
// From an input table containing series on port 0 and another table
// describing densities on port 1 (for instance obtained by applying
// filter  vtkHighestDensityRegionsStatistics, this filter generates
// a table containing all the columns of the input port 0 plus two 2
// components columns containing the bag series to be used by
// vtkFunctionalBagPlot.
//
// .SECTION See Also
// vtkFunctionalBagPlot vtkHighestDensityRegionsStatistics

#ifndef __vtkExtractFunctionalBagPlot_h
#define __vtkExtractFunctionalBagPlot_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"


class VTKFILTERSSTATISTICS_EXPORT vtkExtractFunctionalBagPlot : public vtkTableAlgorithm
{
public:
  static vtkExtractFunctionalBagPlot* New();
  vtkTypeMacro(vtkExtractFunctionalBagPlot, vtkTableAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkExtractFunctionalBagPlot();
  virtual ~vtkExtractFunctionalBagPlot();

  int RequestData(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkExtractFunctionalBagPlot( const vtkExtractFunctionalBagPlot& ); // Not implemented.
  void operator = ( const vtkExtractFunctionalBagPlot& ); // Not implemented.
};

#endif // __vtkExtractFunctionalBagPlot_h
