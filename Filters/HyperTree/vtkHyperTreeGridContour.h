/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridContour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridContour
 * @brief   Extract cells from a hyper tree grid
 * where selected scalar value is within given range.
 *
 *
 * This filter extracts cells from a hyper tree grid that satisfy the
 * following contour: a cell is considered to be within range if its
 * value for the active scalar is within a specified range (inclusive).
 * The output is a vtkPolyData.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkContourFilter
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was revised by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridContour_h
#define vtkHyperTreeGridContour_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkContourValues.h" // Needed for inline methods

#include <vector> // For STL

class vtkBitArray;
class vtkContourHelper;
class vtkDataArray;
class vtkHyperTreeCursor;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkIdList;
class vtkIncrementalPointLocator;
class vtkLine;
class vtkPixel;
class vtkPointData;
class vtkUnsignedCharArray;
class vtkVoxel;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridContour : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridContour* New();
  vtkTypeMacro( vtkHyperTreeGridContour, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* );
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Modified GetMTime Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Methods (inlined) to set / get contour values.
   */
  void SetValue( int, double );
  double GetValue( int );
  double *GetValues();
  void GetValues( double* );
  void SetNumberOfContours( int) ;
  int GetNumberOfContours();
  void GenerateValues( int, double[2] );
  void GenerateValues( int, double, double);
  //@}

protected:
  vtkHyperTreeGridContour();
  ~vtkHyperTreeGridContour() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to generate isocontours of hyper tree grid.
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively decide whether a cell is intersected by a contour
   */
  bool RecursivelyPreProcessTree( vtkHyperTreeGridCursor* );

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Storage for contour values.
   */
  vtkContourValues* ContourValues;

  /**
   * Storage for pre-selected cells to be processed
   */
  vtkBitArray* SelectedCells;

  /**
   * Sign of isovalue if cell not treated
   */
  vtkBitArray** CellSigns;

  /**
   * Spatial locator to merge points.
   */
  vtkIncrementalPointLocator* Locator;

  //@{
  /**
   * Pointers needed to perform isocontouring
   */
  vtkContourHelper* Helper;
  vtkDataArray* CellScalars;
  vtkLine* Line;
  vtkPixel* Pixel;
  vtkVoxel* Voxel;
  vtkIdList* Leaves;
  //@}

  /**
   * Storage for signs relative to current contour value
   */
  std::vector<bool> Signs;

  /**
   * Keep track of current index in output polydata
   */
  vtkIdType CurrentId;

  /**
   * Keep track of selected input scalars
   */
 vtkDataArray* InScalars;

private:
  vtkHyperTreeGridContour(const vtkHyperTreeGridContour&) = delete;
  void operator=(const vtkHyperTreeGridContour&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkHyperTreeGridContour::SetValue( int i, double value )
  { this->ContourValues->SetValue( i, value ); }

/**
 * Get the ith contour value.
 */
inline double vtkHyperTreeGridContour::GetValue( int i )
  { return this->ContourValues->GetValue( i );}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double *vtkHyperTreeGridContour::GetValues()
  { return this->ContourValues->GetValues(); }

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkHyperTreeGridContour::GetValues( double* contourValues )
  { this->ContourValues->GetValues( contourValues ); }

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkHyperTreeGridContour::SetNumberOfContours( int number )
  { this->ContourValues->SetNumberOfContours( number ); }

/**
 * Get the number of contours in the list of contour values.
 */
inline int vtkHyperTreeGridContour::GetNumberOfContours()
  { return this->ContourValues->GetNumberOfContours(); }

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkHyperTreeGridContour::GenerateValues( int numContours,
                                                     double range[2] )
  { this->ContourValues->GenerateValues( numContours, range ); }

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkHyperTreeGridContour::GenerateValues( int numContours,
                                                     double rangeStart,
                                                     double rangeEnd )
  { this->ContourValues->GenerateValues( numContours, rangeStart, rangeEnd ); }

#endif /* vtkHyperTreeGridContour_h */
