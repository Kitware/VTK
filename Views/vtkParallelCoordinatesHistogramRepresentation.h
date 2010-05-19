/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesHistogramRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkParallelCoordinatesHistogramRepresentation - Data representation 
//  that takes generic multivariate data and produces a parallel coordinates plot.
//  This plot optionally can draw a histogram-based plot summary.
//
// .SECTION Description
//  A parallel coordinates plot represents each variable in a multivariate
//  data set as a separate axis.  Individual samples of that data set are
//  represented as a polyline that pass through each variable axis at 
//  positions that correspond to data values.  This class can generate
//  parallel coordinates plots identical to its superclass 
//  (vtkParallelCoordinatesRepresentation) and has the same interaction
//  styles.
//
//  In addition to the standard parallel coordinates plot, this class also
//  can draw a histogram summary of the parallel coordinates plot.  
//  Rather than draw every row in an input data set, first it computes
//  a 2D histogram for all neighboring variable axes, then it draws
//  bar (thickness corresponds to bin size) for each bin the histogram
//  with opacity weighted by the number of rows contained in the bin.
//  The result is essentially a density map.  
//
//  Because this emphasizes dense regions over sparse outliers, this class
//  also uses a vtkComputeHistogram2DOutliers instance to identify outlier
//  table rows and draws those as standard parallel coordinates lines.
//
// .SECTION See Also
//  vtkParallelCoordinatesView vtkParallelCoordinatesRepresentation
//  vtkExtractHistogram2D vtkComputeHistogram2DOutliers
// 
// .SECTION Thanks
//  Developed by David Feng at Sandia National Laboratories

#ifndef __vtkParallelCoordinatesHistogramRepresentation_h
#define __vtkParallelCoordinatesHistogramRepresentation_h

#include "vtkParallelCoordinatesRepresentation.h"

class vtkComputeHistogram2DOutliers;
class vtkPairwiseExtractHistogram2D;
class vtkExtractHistogram2D;
class vtkInformationVector;
class vtkLookupTable;

class VTK_VIEWS_EXPORT vtkParallelCoordinatesHistogramRepresentation : public vtkParallelCoordinatesRepresentation
{
public:
  static vtkParallelCoordinatesHistogramRepresentation* New();
  vtkTypeMacro(vtkParallelCoordinatesHistogramRepresentation, vtkParallelCoordinatesRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Whether to use the histogram rendering mode or the superclass's line rendering mode
  virtual void SetUseHistograms(int);
  vtkGetMacro(UseHistograms,int);
  vtkBooleanMacro(UseHistograms,int);

  // Description:
  // Whether to compute and show outlier lines
  virtual void SetShowOutliers(int);
  vtkGetMacro(ShowOutliers,int);
  vtkBooleanMacro(ShowOutliers,int);

  // Description:
  // Control over the range of the lookup table used to draw the histogram quads.
  vtkSetVector2Macro(HistogramLookupTableRange,double);
  vtkGetVector2Macro(HistogramLookupTableRange,double);

  //BTX
  // Description:
  // The number of histogram bins on either side of each pair of axes.
  void SetNumberOfHistogramBins(int,int);
  void SetNumberOfHistogramBins(int*);
  vtkGetVector2Macro(NumberOfHistogramBins,int);
  //ETX

  // Description:
  // Target maximum number of outliers to be drawn, although not guaranteed.
  void SetPreferredNumberOfOutliers(int);
  vtkGetMacro(PreferredNumberOfOutliers,int);

  // Description:
  // Calls superclass swap, and assures that only histograms affected by the 
  // swap get recomputed.
  virtual int SwapAxisPositions(int position1, int position2);

  // Description:
  // Calls the superclass method, and assures that only the two histograms
  // affect by this call get recomputed.
  virtual int SetRangeAtPosition(int position, double range[2]);

protected:
  vtkParallelCoordinatesHistogramRepresentation();
  virtual ~vtkParallelCoordinatesHistogramRepresentation();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  virtual bool AddToView(vtkView* view);

  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Flag deciding if histograms will be drawn.
  int UseHistograms;

  // Description:
  // The range applied to the lookup table used to draw histogram quads
  double HistogramLookupTableRange[2];

  // Description:
  // How many bins are used during the 2D histogram computation
  int NumberOfHistogramBins[2];

  //BTX
  vtkSmartPointer<vtkPairwiseExtractHistogram2D> HistogramFilter;
  vtkSmartPointer<vtkLookupTable> HistogramLookupTable;
  //ETX

  // Description:
  // Whether or not to draw outlier lines
  int ShowOutliers;

  // Description:
  // How many outlier lines to draw, approximately.
  int PreferredNumberOfOutliers;

  //BTX
  vtkSmartPointer<vtkComputeHistogram2DOutliers> OutlierFilter;
  vtkSmartPointer<vtkPolyData> OutlierData;
  vtkSmartPointer<vtkPolyDataMapper2D> OutlierMapper;
  vtkSmartPointer<vtkActor2D> OutlierActor;
  //ETX
  
  // Description:
  // Correctly forwards the superclass call to draw lines to the internal
  // PlaceHistogramLineQuads call.
  virtual int PlaceLines(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot);

  // Description:
  // Correctly forwards the superclass call to draw curves to the internal
  // PlaceHistogramLineCurves call.
  virtual int PlaceCurves(vtkPolyData* polyData, vtkTable* data, vtkIdTypeArray* idsToPlot);

  // Description:
  // Draw a selection node referencing the row ids of a table into a poly data object.
  virtual int PlaceSelection(vtkPolyData* polyData, vtkTable* data, vtkSelectionNode* selectionNode);

  // Description:
  // Take the input 2D histogram images and draw one quad for each bin 
  virtual int PlaceHistogramLineQuads(vtkPolyData* polyData);

  // Description:
  // Take the input 2D histogram images and draw one triangle strip that
  // is the curved version of the regular quad drawn via PlaceHistogramLineQuads
  virtual int PlaceHistogramCurveQuads(vtkPolyData* polyData);

  // Description:
  // Compute the number of axes and their individual ranges, as well
  // as histograms if requested.
  virtual int ComputeDataProperties();
  virtual int UpdatePlotProperties(vtkStringArray*);

  // Description:
  // Access the input data object containing the histograms and 
  // pull out the image data for the idx'th histogram.
  virtual vtkImageData* GetHistogramImage(int idx);

  // Description:
  // get the table containing just the outlier rows from the input table.
  virtual vtkTable* GetOutlierData();
private:
  vtkParallelCoordinatesHistogramRepresentation(const vtkParallelCoordinatesHistogramRepresentation&); // Not implemented
  void operator=(const vtkParallelCoordinatesHistogramRepresentation&);   // Not implemented
};

#endif
