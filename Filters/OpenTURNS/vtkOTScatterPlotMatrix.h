/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOTScatterPlotMatrix
 * @brief   container for a matrix of charts.
 *
 *
 * This class specialize vtkScatterPlotMatrix by adding a density map
 * on the chart, computed with OpenTURNS
 *
 * @sa
 * vtkScatterPlotMatrix vtkOTDensityMap
 */

#ifndef vtkOTScatterPlotMatrix_h
#define vtkOTScatterPlotMatrix_h

#include "vtkFiltersOpenTURNSModule.h" // For export macro
#include "vtkScatterPlotMatrix.h"
#include "vtkSmartPointer.h" // For SmartPointer

class vtkOTDensityMap;
class vtkScalarsToColors;

class VTKFILTERSOPENTURNS_EXPORT vtkOTScatterPlotMatrix : public vtkScatterPlotMatrix
{
public:
  vtkTypeMacro(vtkOTScatterPlotMatrix, vtkScatterPlotMatrix);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static vtkOTScatterPlotMatrix* New();

  /**
   * Set the visibility of density map for the specific plotType, false by default
   */
  void SetDensityMapVisibility(int plotType, bool visible);

  /**
   * Set the density line size for the specified plotType, 2 by default
   */
  void SetDensityLineSize(int plotType, float size);

  /**
   * Set the color for the specified plotType, automatically distributed on HSV by default
   */
  void SetDensityMapColor(int plotType, unsigned int densityLineIndex, const vtkColor4ub& color);

  //@{
  /**
   * Get/Set a custom color transfer function.
   * If none is provided, a default one will be applied based on the range of the density.
   */
  void SetTransferFunction(vtkScalarsToColors* stc);
  vtkScalarsToColors* GetTransferFunction();
  //@}

protected:
  vtkOTScatterPlotMatrix();
  ~vtkOTScatterPlotMatrix() override;

  /**
   * Add a density map as a supplementary plot,
   * with provided row and column, computed with OpenTURNS
   * if DensityMapVisibility is true and we are not animating
   */
  virtual void AddSupplementaryPlot(vtkChart* chart, int plotType, vtkStdString row,
    vtkStdString column, int plotCorner = 0) override;

private:
  vtkOTScatterPlotMatrix(const vtkOTScatterPlotMatrix&) = delete;
  void operator=(const vtkOTScatterPlotMatrix&) = delete;

  class DensityMapSettings;
  std::map<int, DensityMapSettings*> DensityMapsSettings;
  typedef std::map<std::pair<vtkStdString, vtkStdString>, vtkSmartPointer<vtkOTDensityMap> >
    DensityMapCacheMap;
  DensityMapCacheMap DensityMapCache;

  vtkSmartPointer<vtkScalarsToColors> TransferFunction;
};

#endif // vtkOTScatterPlotMatrix_h
