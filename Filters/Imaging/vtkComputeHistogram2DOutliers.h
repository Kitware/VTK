/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeHistogram2DOutliers.h

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
// .NAME vtkComputeHistogram2DOutliers - compute the outliers in a set
//  of 2D histograms and extract the corresponding row data.
//
// .SECTION Description
//  This class takes a table and one or more vtkImageData histograms as input
//  and computes the outliers in that data.  In general it does so by
//  identifying histogram bins that are removed by a median (salt and pepper)
//  filter and below a threshold.  This threshold is automatically identified
//  to retrieve a number of outliers close to a user-determined value.  This
//  value is set by calling SetPreferredNumberOfOutliers(int).
//
//  The image data input can come either as a multiple vtkImageData via the
//  repeatable INPUT_HISTOGRAM_IMAGE_DATA port, or as a single
//  vtkMultiBlockDataSet containing vtkImageData objects as blocks.  One
//  or the other must be set, not both (or neither).
//
//  The output can be retrieved as a set of row ids in a vtkSelection or
//  as a vtkTable containing the actual outlier row data.
//
// .SECTION See Also
//  vtkExtractHistogram2D vtkPComputeHistogram2DOutliers
//
// .SECTION Thanks
//  Developed by David Feng at Sandia National Laboratories
//------------------------------------------------------------------------------
#ifndef vtkComputeHistogram2DOutliers_h
#define vtkComputeHistogram2DOutliers_h
//------------------------------------------------------------------------------
#include "vtkFiltersImagingModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

//------------------------------------------------------------------------------
class vtkCollection;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkImageData;
class vtkTable;
//------------------------------------------------------------------------------
class VTKFILTERSIMAGING_EXPORT vtkComputeHistogram2DOutliers : public vtkSelectionAlgorithm
{
public:
  static vtkComputeHistogram2DOutliers* New();
  vtkTypeMacro(vtkComputeHistogram2DOutliers, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(PreferredNumberOfOutliers,int);
  vtkGetMacro(PreferredNumberOfOutliers,int);

  //
  vtkTable* GetOutputTable();
//BTX
  enum InputPorts
  {
    INPUT_TABLE_DATA=0,
    INPUT_HISTOGRAMS_IMAGE_DATA,
    INPUT_HISTOGRAMS_MULTIBLOCK
  };
  enum OutputPorts
  {
    OUTPUT_SELECTED_ROWS=0,
    OUTPUT_SELECTED_TABLE_DATA
  };
//ETX

  // Description:
  // Set the source table data, from which data will be filtered.
  void SetInputTableConnection(vtkAlgorithmOutput* cxn)
  { this->SetInputConnection(INPUT_TABLE_DATA,cxn); }

  // Description:
  // Set the input histogram data as a (repeatable) vtkImageData
  void SetInputHistogramImageDataConnection(vtkAlgorithmOutput* cxn)
  { this->SetInputConnection(INPUT_HISTOGRAMS_IMAGE_DATA,cxn); }

  // Description:
  // Set the input histogram data as a vtkMultiBlockData set
  // containing multiple vtkImageData objects.
  void SetInputHistogramMultiBlockConnection(vtkAlgorithmOutput* cxn)
  { this->SetInputConnection(INPUT_HISTOGRAMS_MULTIBLOCK,cxn); }

protected:
  vtkComputeHistogram2DOutliers();
  ~vtkComputeHistogram2DOutliers();

  int PreferredNumberOfOutliers;
  vtkTimeStamp BuildTime;

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  // Description:
  // Compute the thresholds (essentially bin extents) that contain outliers for
  // a collection of vtkImageData histograms.
  virtual int ComputeOutlierThresholds(vtkCollection* histograms, vtkCollection* thresholds);

  // Description:
  // Compute the thresholds (bin extents) that contain outliers for a single vtkImageData histogram
  virtual int ComputeOutlierThresholds(vtkImageData* histogram, vtkDoubleArray* thresholds, double threshold);

  // Description:
  // Take a set of range thresholds (bin extents) and filter out rows from the input table data that
  // fits inside those thresholds.
  virtual int FillOutlierIds(vtkTable* data, vtkCollection* thresholds, vtkIdTypeArray* rowIds, vtkTable* outTable);
private:
  vtkComputeHistogram2DOutliers(const vtkComputeHistogram2DOutliers&); // Not implemented
  void operator=(const vtkComputeHistogram2DOutliers&);   // Not implemented
};

#endif
