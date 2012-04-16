/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeDualGridContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeDualGridContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// use of unsigned short to hold level index limits tree depth to 16.
//
// To use this filter you must specify one or more contour values.
// You can either use the method SetValue() to specify each contour
// value, or use GenerateValues() to generate a series of evenly
// spaced contours. It is also possible to accelerate the operation of
// this filter (at the cost of extra memory) by using a
// vtkScalarTree. A scalar tree is used to quickly locate cells that
// contain a contour surface. This is especially effective if multiple
// contours are being extracted. If you want to use a scalar tree,
// invoke the method UseScalarTreeOn().

// .SECTION See Also
// vtkMarchingContourFilter vtkKitwareContourFilter
// vtkMarchingCubes vtkSliceCubes vtkDividingCubes vtkMarchingSquares
// vtkImageMarchingCubes

#ifndef __vtkHyperOctreeDualGridContourFilter_h
#define __vtkHyperOctreeDualGridContourFilter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods
#include "vtkCutter.h" // for VTK_SORT_BY_VALUE

class vtkHyperOctree;
class vtkTetra;
class vtkHyperOctreeCursor;
class vtkHyperOctreeLightWeightCursor;

class vtkDataSetAttributes;
class vtkUnstructuredGrid;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
class vtkBitArray;
class vtkIncrementalPointLocator;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeDualGridContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeDualGridContourFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkHyperOctreeDualGridContourFilter *New();

  // Description:
  // Methods to set / get contour values.

  // Description:
  // Set a particular contour value at contour number i. The index i ranges
  // between 0<=i<NumberOfContours.
  void SetValue(int i, double value)
    {
      this->ContourValues->SetValue(i,value);
    }

  // Description:
  // Get the ith contour value.
  double GetValue(int i)
    {
      return this->ContourValues->GetValue(i);
    }

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  double *GetValues()
    {
      return this->ContourValues->GetValues();
    }

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(double *contourValues)
    {
      this->ContourValues->GetValues(contourValues);
    }

  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number)
    {
      this->ContourValues->SetNumberOfContours(number);
    }

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours()
    {
      return this->ContourValues->GetNumberOfContours();
    }

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double range[2])
    {
      this->ContourValues->GenerateValues(numContours, range);
    }

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double
                      rangeStart, double rangeEnd)
    {
      this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
    }

  // Description:
  // Modified GetMTime Because we delegate to vtkContourValues
  unsigned long GetMTime();

  // Description:
  // Set / get a spatial locator for merging points. By default,
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is
  // specified. The locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkHyperOctreeDualGridContourFilter();
  ~vtkHyperOctreeDualGridContourFilter();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Description:
  // Do the recursive contour of the node pointed by Cursor.
  void ContourNode();

  void TraverseNeighborhoodRecursively(
                     vtkHyperOctreeLightWeightCursor* neighborhood,
                     unsigned short* xyzIds);
  void EvaluatePoint(vtkHyperOctreeLightWeightCursor* neighborhood,
                     unsigned short* xyzIds);

  void ContourNode1D();

  vtkContourValues *ContourValues;
  vtkIncrementalPointLocator *Locator;

  vtkHyperOctree *Input;
  vtkPolyData *Output;

  vtkCellArray *NewPolys;

  vtkDataSetAttributes *InPD;
  vtkDataSetAttributes *OutPD;
  vtkDataArray *InScalars;
  // To compute points on the fly.
  // These are set to the input origin and size.
  double Origin[3];
  double Size[3];

  // This is a table for traversing a neighborhood down an octree.
  // 8 children x 8 cursors
  // First three bits encode the child,  rest encode the cursor id.
  // 8xCursorId + childId.
  unsigned char NeighborhoodTraversalTable[64];
  void GenerateTraversalTable();

private:
  vtkHyperOctreeDualGridContourFilter(const vtkHyperOctreeDualGridContourFilter&);  // Not implemented.
  void operator=(const vtkHyperOctreeDualGridContourFilter&);  // Not implemented.
};
#endif
