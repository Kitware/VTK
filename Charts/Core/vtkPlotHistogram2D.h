/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk2DHistogramItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtk2DHistogramItem
 * @brief   2D histogram item.
 *
 *
 *
*/

#ifndef vtkPlotHistogram2D_h
#define vtkPlotHistogram2D_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"
#include "vtkSmartPointer.h"  // Needed for SP ivars
#include "vtkRect.h"          // Needed for vtkRectf

class vtkImageData;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT vtkPlotHistogram2D : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotHistogram2D, vtkPlot);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static vtkPlotHistogram2D *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever it needs to be drawn.
   */
  bool Paint(vtkContext2D *painter) override;

  /**
   * Set the input, we are expecting a vtkImageData with just one component,
   * this would normally be a float or a double. It will be passed to the other
   * functions as a double to generate a color.
   */
  virtual void SetInputData(vtkImageData *data, vtkIdType z = 0);
  void SetInputData(vtkTable*) override { }
  void SetInputData(vtkTable*, const vtkStdString&, const vtkStdString&) override { }

  /**
   * Get the input table used by the plot.
   */
  vtkImageData * GetInputImageData();

  /**
   * Set the color transfer function that will be used to generate the 2D
   * histogram.
   */
  void SetTransferFunction(vtkScalarsToColors *transfer);

  /**
   * Get the color transfer function that is used to generate the histogram.
   */
  vtkScalarsToColors * GetTransferFunction();

  void GetBounds(double bounds[4]) override;

  virtual void SetPosition(const vtkRectf& pos);
  virtual vtkRectf GetPosition();

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored.
   * The member variable TooltipLabelFormat can be set as a
   * printf-style string to build custom tooltip labels from,
   * and may contain:
   * An empty string generates the default tooltip labels.
   * The following case-sensitive format tags (without quotes) are recognized:
   * '%x' The X position of the histogram cell
   * '%y' The Y position of the histogram cell
   * '%v' The scalar value of the histogram cell
   * Note: the %i and %j tags are valid only if there is a 1:1 correspondence
   * between individual histogram cells and axis tick marks
   * '%i' The X axis tick label for the histogram cell
   * '%j' The Y axis tick label for the histogram cell
   * Any other characters or unrecognized format tags are printed in the
   * tooltip label verbatim.
   */
  vtkStdString GetTooltipLabel(const vtkVector2d &plotPos,
                                       vtkIdType seriesIndex,
                                       vtkIdType segmentIndex) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns an index between 0 and (number of histogram cells - 1), or -1.
   * The index 0 is at cell x=0, y=0 of the histogram, and the index increases
   * in a minor fashon with x and in a major fashon with y.
   * The referent of "location" is set to the x and y integer indices of the
   * histogram cell.
   */
  vtkIdType GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tolerance,
                                    vtkVector2f* location) override;

protected:
  vtkPlotHistogram2D();
  ~vtkPlotHistogram2D() override;

  /**
   * Where all the magic happens...
   */
  void GenerateHistogram();

  vtkSmartPointer<vtkImageData> Input;
  vtkSmartPointer<vtkImageData> Output;
  vtkSmartPointer<vtkScalarsToColors> TransferFunction;
  vtkRectf Position;

private:
  vtkPlotHistogram2D(const vtkPlotHistogram2D &) = delete;
  void operator=(const vtkPlotHistogram2D &) = delete;

};

#endif //vtkPlotHistogram2D_h
