// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkParallelCoordinatesHistogramRepresentation
 * @brief   Data representation
 *  that takes generic multivariate data and produces a parallel coordinates plot.
 *  This plot optionally can draw a histogram-based plot summary.
 *
 *
 *  A parallel coordinates plot represents each variable in a multivariate
 *  data set as a separate axis.  Individual samples of that data set are
 *  represented as a polyline that pass through each variable axis at
 *  positions that correspond to data values.  This class can generate
 *  parallel coordinates plots identical to its superclass
 *  (vtkParallelCoordinatesRepresentation) and has the same interaction
 *  styles.
 *
 *  In addition to the standard parallel coordinates plot, this class also
 *  can draw a histogram summary of the parallel coordinates plot.
 *  Rather than draw every row in an input data set, first it computes
 *  a 2D histogram for all neighboring variable axes, then it draws
 *  bar (thickness corresponds to bin size) for each bin the histogram
 *  with opacity weighted by the number of rows contained in the bin.
 *  The result is essentially a density map.
 *
 *  Because this emphasizes dense regions over sparse outliers, this class
 *  also uses a vtkComputeHistogram2DOutliers instance to identify outlier
 *  table rows and draws those as standard parallel coordinates lines.
 *
 * @sa
 *  vtkParallelCoordinatesView vtkParallelCoordinatesRepresentation
 *  vtkExtractHistogram2D vtkComputeHistogram2DOutliers
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 */

#ifndef vtkParallelCoordinatesHistogramRepresentation_h
#define vtkParallelCoordinatesHistogramRepresentation_h

#include "vtkParallelCoordinatesRepresentation.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkComputeHistogram2DOutliers;
class vtkPairwiseExtractHistogram2D;
class vtkExtractHistogram2D;
class vtkInformationVector;
class vtkLookupTable;

class VTKVIEWSINFOVIS_EXPORT vtkParallelCoordinatesHistogramRepresentation
  : public vtkParallelCoordinatesRepresentation
{
public:
  static vtkParallelCoordinatesHistogramRepresentation* New();
  vtkTypeMacro(vtkParallelCoordinatesHistogramRepresentation, vtkParallelCoordinatesRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Apply the theme to this view.
   */
  void ApplyViewTheme(vtkViewTheme* theme) override;

  ///@{
  /**
   * Whether to use the histogram rendering mode or the superclass's line rendering mode
   */
  virtual void SetUseHistograms(vtkTypeBool);
  vtkGetMacro(UseHistograms, vtkTypeBool);
  vtkBooleanMacro(UseHistograms, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Whether to compute and show outlier lines
   */
  virtual void SetShowOutliers(vtkTypeBool);
  vtkGetMacro(ShowOutliers, vtkTypeBool);
  vtkBooleanMacro(ShowOutliers, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Control over the range of the lookup table used to draw the histogram quads.
   */
  vtkSetVector2Macro(HistogramLookupTableRange, double);
  vtkGetVector2Macro(HistogramLookupTableRange, double);
  ///@}

  ///@{
  /**
   * The number of histogram bins on either side of each pair of axes.
   */
  void SetNumberOfHistogramBins(int, int);
  void SetNumberOfHistogramBins(int*);
  vtkGetVector2Macro(NumberOfHistogramBins, int);
  ///@}

  ///@{
  /**
   * Target maximum number of outliers to be drawn, although not guaranteed.
   */
  void SetPreferredNumberOfOutliers(int);
  vtkGetMacro(PreferredNumberOfOutliers, int);
  ///@}

  /**
   * Calls superclass swap, and assures that only histograms affected by the
   * swap get recomputed.
   */
  int SwapAxisPositions(int position1, int position2) override;

  /**
   * Calls the superclass method, and assures that only the two histograms
   * affect by this call get recomputed.
   */
  int SetRangeAtPosition(int position, double range[2]) override;

protected:
  vtkParallelCoordinatesHistogramRepresentation();
  ~vtkParallelCoordinatesHistogramRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AddToView(vtkView* view) override;

  bool RemoveFromView(vtkView* view) override;

  /**
   * Flag deciding if histograms will be drawn.
   */
  vtkTypeBool UseHistograms;

  /**
   * The range applied to the lookup table used to draw histogram quads
   */
  double HistogramLookupTableRange[2];

  /**
   * How many bins are used during the 2D histogram computation
   */
  int NumberOfHistogramBins[2];

  vtkSmartPointer<vtkPairwiseExtractHistogram2D> HistogramFilter;
  vtkSmartPointer<vtkLookupTable> HistogramLookupTable;

  /**
   * Whether or not to draw outlier lines
   */
  vtkTypeBool ShowOutliers;

  /**
   * How many outlier lines to draw, approximately.
   */
  int PreferredNumberOfOutliers;

  vtkSmartPointer<vtkComputeHistogram2DOutliers> OutlierFilter;
  vtkSmartPointer<vtkPolyData> OutlierData;
  vtkSmartPointer<vtkPolyDataMapper2D> OutlierMapper;
  vtkSmartPointer<vtkActor2D> OutlierActor;

  /**
   * Correctly forwards the superclass call to draw lines to the internal
   * PlaceHistogramLineQuads call.
   */
  int PlaceLines(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot) override;

  /**
   * Correctly forwards the superclass call to draw curves to the internal
   * PlaceHistogramLineCurves call.
   */
  int PlaceCurves(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot) override;

  /**
   * Draw a selection node referencing the row ids of a table into a poly data object.
   */
  int PlaceSelection(
    vtkPolyData* polyData, vtkTable* data, vtkSelectionNode* selectionNode) override;

  /**
   * Take the input 2D histogram images and draw one quad for each bin
   */
  virtual int PlaceHistogramLineQuads(vtkPolyData* polyData);

  /**
   * Take the input 2D histogram images and draw one triangle strip that
   * is the curved version of the regular quad drawn via PlaceHistogramLineQuads
   */
  virtual int PlaceHistogramCurveQuads(vtkPolyData* polyData);

  ///@{
  /**
   * Compute the number of axes and their individual ranges, as well
   * as histograms if requested.
   */
  int ComputeDataProperties() override;
  int UpdatePlotProperties(vtkStringArray*) override;
  ///@}

  /**
   * Access the input data object containing the histograms and
   * pull out the image data for the idx'th histogram.
   */
  virtual vtkImageData* GetHistogramImage(int idx);

  /**
   * get the table containing just the outlier rows from the input table.
   */
  virtual vtkTable* GetOutlierData();

private:
  vtkParallelCoordinatesHistogramRepresentation(
    const vtkParallelCoordinatesHistogramRepresentation&) = delete;
  void operator=(const vtkParallelCoordinatesHistogramRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
