/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBivariateLinearTableThreshold.h

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
// .NAME vtkBivariateLinearTableThreshold - performs line-based thresholding
// for vtkTable data.
//
// .SECTION Description
// Class for filtering the rows of a two numeric columns of a vtkTable.  The
// columns are treated as the two variables of a line.  This filter will
// then iterate through the rows of the table determining if X,Y values pairs
// are above/below/between/near one or more lines.
//
// The "between" mode checks to see if a row is contained within the convex
// hull of all of the specified lines.  The "near" mode checks if a row is
// within a distance threshold two one of the specified lines.  This class
// is used in conjunction with various plotting classes, so it is useful
// to rescale the X,Y axes to a particular range of values.  Distance
// comparisons can be performed in the scaled space by setting the CustomRanges
// ivar and enabling UseNormalizedDistance.

#ifndef __vtkBivariateLinearTableThreshold__h
#define __vtkBivariateLinearTableThreshold__h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include "vtkSmartPointer.h"  //Required for smart pointer internal ivars

class vtkDataArrayCollection;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkTable;

class VTKFILTERSSTATISTICS_EXPORT vtkBivariateLinearTableThreshold : public vtkTableAlgorithm
{
public:
  static vtkBivariateLinearTableThreshold* New();
  vtkTypeMacro(vtkBivariateLinearTableThreshold, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Include the line in the threshold.  Essentially whether the threshold operation
  // uses > versus >=.
  vtkSetMacro(Inclusive,int);
  vtkGetMacro(Inclusive,int);

  // Description:
  // Add a numeric column to the pair of columns to be thresholded.  Call twice.
  void AddColumnToThreshold(vtkIdType column, vtkIdType component);

  // Description:
  // Return how many columns have been added.  Hopefully 2.
  int GetNumberOfColumnsToThreshold();

  // Description:
  // Return the column number from the input table for the idx'th added column.
  void GetColumnToThreshold(vtkIdType idx, vtkIdType& column, vtkIdType& component);

  // Description:
  // Reset the columns to be thresholded.
  void ClearColumnsToThreshold();

  // Description:
  // Get the output as a table of row ids.
  vtkIdTypeArray* GetSelectedRowIds(int selection=0);

  //BTX
  enum OutputPorts
  {
    OUTPUT_ROW_IDS=0,
    OUTPUT_ROW_DATA
  };
  enum LinearThresholdType
  {
    BLT_ABOVE=0,
    BLT_BELOW,
    BLT_NEAR,
    BLT_BETWEEN
  };
  //ETX

  // Description:
  // Reset the columns to threshold, column ranges, etc.
  void Initialize();

  // Description:
  // Add a line for thresholding from two x,y points.
  void AddLineEquation(double* p1, double* p2);

  // Description:
  // Add a line for thresholding in point-slope form.
  void AddLineEquation(double* p, double slope);

  // Description:
  // Add a line for thresholding in implicit form (ax + by + c = 0)
  void AddLineEquation(double a, double b, double c);

  // Description:
  // Reset the list of line equations.
  void ClearLineEquations();

  // Description:
  // Set the threshold type.  Above: find all rows that are above the specified
  // lines.  Below: find all rows that are below the specified lines.  Near:
  // find all rows that are near the specified lines.  Between: find all rows
  // that are between the specified lines.
  vtkGetMacro(LinearThresholdType,int);
  vtkSetMacro(LinearThresholdType,int);
  void SetLinearThresholdTypeToAbove() { this->SetLinearThresholdType(vtkBivariateLinearTableThreshold::BLT_ABOVE); }
  void SetLinearThresholdTypeToBelow() { this->SetLinearThresholdType(vtkBivariateLinearTableThreshold::BLT_BELOW); }
  void SetLinearThresholdTypeToNear() { this->SetLinearThresholdType(vtkBivariateLinearTableThreshold::BLT_NEAR); }
  void SetLinearThresholdTypeToBetween() { this->SetLinearThresholdType(vtkBivariateLinearTableThreshold::BLT_BETWEEN); }

  // Description:
  // Manually access the maximum/minimum x,y values.  This is used in
  // conjunction with UseNormalizedDistance when determining if a row
  // passes the threshold.
  vtkSetVector2Macro(ColumnRanges,double);
  vtkGetVector2Macro(ColumnRanges,double);

  // Description:
  // The Cartesian distance within which a point will pass the near threshold.
  vtkSetMacro(DistanceThreshold,double);
  vtkGetMacro(DistanceThreshold,double);

  // Description:
  // Renormalize the space of the data such that the X and Y axes are
  // "square" over the specified ColumnRanges.  This essentially scales
  // the data space so that ColumnRanges[1]-ColumnRanges[0] = 1.0 and
  // ColumnRanges[3]-ColumnRanges[2] = 1.0.  Used for scatter plot distance
  // calculations.  Be sure to set DistanceThreshold accordingly, when used.
  vtkSetMacro(UseNormalizedDistance,int);
  vtkGetMacro(UseNormalizedDistance,int);
  vtkBooleanMacro(UseNormalizedDistance,int);

  // Description:
  // Convert the two-point line formula to implicit form.
  static void ComputeImplicitLineFunction(double* p1, double* p2, double* abc);

  // Description:
  // Convert the point-slope line formula to implicit form.
  static void ComputeImplicitLineFunction(double* p, double slope, double* abc);

protected:
  vtkBivariateLinearTableThreshold();
  virtual ~vtkBivariateLinearTableThreshold();

  double ColumnRanges[2];
  double DistanceThreshold;
  int Inclusive;
  int LinearThresholdType;
  int NumberOfLineEquations;
  int UseNormalizedDistance;

  //BTX
  vtkSmartPointer<vtkDoubleArray> LineEquations;
  class Internals;
  Internals* Implementation;
  //ETX

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  // Description:
  // Apply the current threshold to a vtkTable.  Fills acceptedIds on success.
  virtual int ApplyThreshold(vtkTable* tableToThreshold, vtkIdTypeArray* acceptedIds);

  // Description:
  // Determine if x,y is above all specified lines.
  int ThresholdAbove(double x, double y);

  // Description:
  // Determine if x,y is below all specified lines.
  int ThresholdBelow(double x, double y);

  // Description:
  // Determine if x,y is near ONE specified line (not all).
  int ThresholdNear(double x, double y);

  // Description:
  // Determine if x,y is between ANY TWO of the specified lines.
  int ThresholdBetween(double x, double y);
private:
  vtkBivariateLinearTableThreshold(const vtkBivariateLinearTableThreshold&); // Not implemented
  void operator=(const vtkBivariateLinearTableThreshold&); // Not implemented
};

#endif
