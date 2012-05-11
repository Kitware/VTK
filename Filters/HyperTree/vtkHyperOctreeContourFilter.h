/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkContourFilter is a filter that takes as input any dataset and
// generates on output isosurfaces and/or isolines. The exact form
// of the output depends upon the dimensionality of the input data.
// Data consisting of 3D cells will generate isosurfaces, data
// consisting of 2D cells will generate isolines, and data with 1D
// or 0D cells will generate isopoints. Combinations of output type
// are possible if the input dimension is mixed.
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

// .SECTION Caveats
// For unstructured data or structured grids, normals and gradients
// are not computed. Use vtkPolyDataNormals to compute the surface
// normals.

// .SECTION See Also
// vtkMarchingContourFilter vtkKitwareContourFilter
// vtkMarchingCubes vtkSliceCubes vtkDividingCubes vtkMarchingSquares
// vtkImageMarchingCubes

#ifndef __vtkHyperOctreeContourFilter_h
#define __vtkHyperOctreeContourFilter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods
#include "vtkCutter.h" // for VTK_SORT_BY_VALUE

class vtkIncrementalPointLocator;
class vtkHyperOctree;
class vtkOrderedTriangulator;
class vtkTetra;
class vtkHyperOctreeCursor;

class vtkUnstructuredGrid;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
class vtkHyperOctreeContourPointsGrabber;
class vtkBitArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeContourFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkHyperOctreeContourFilter *New();

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
  vtkHyperOctreeContourFilter();
  ~vtkHyperOctreeContourFilter();

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

  // Description:
  // (i,j,k) are point coordinates at last level
  double ComputePointValue(int ptIndices[3]);

  void ContourNode1D();

  vtkContourValues *ContourValues;
  vtkIncrementalPointLocator *Locator;

  vtkIdList *CellPts; // for 2D case

  vtkHyperOctree *Input;
  vtkPolyData *Output;

  vtkCellArray *NewVerts;
  vtkCellArray *NewLines;
  vtkCellArray *NewPolys;

  vtkCellData *InCD;
  vtkPointData *InPD;
  vtkCellData *OutCD;
  vtkPointData *OutPD;
  vtkOrderedTriangulator *Triangulator;

  vtkHyperOctreeCursor *Sibling; // to avoid allocation in the loop


  vtkDoubleArray *CellScalars;
  vtkTetra *Tetra;
  vtkDoubleArray *TetScalars;

  vtkPolygon *Polygon;

  vtkHyperOctreeCursor *Cursor;
  vtkHyperOctreeCursor *NeighborCursor;

  vtkIdType CellTypeCounter[65536]; // up-to-65536 points per octant
  vtkIdType TotalCounter;
  vtkIdType TemplateCounter; // record the number of octants that succceed
  // to use the template triangulator

  vtkDataArray *InScalars;
  vtkHyperOctreeContourPointsGrabber *Grabber;

  vtkDoubleArray *PointScalars;
  int SortBy;
  int Iter; // iterate over contour values in case of VTK_SORT_BY_CELL

  vtkLine *Line;
  double LeftValue;
  double LeftCoord;

  //BTX
  friend class vtkHyperOctreeContourPointsGrabber;
  //ETX
private:
  vtkHyperOctreeContourFilter(const vtkHyperOctreeContourFilter&);  // Not implemented.
  void operator=(const vtkHyperOctreeContourFilter&);  // Not implemented.
};
#endif
