/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCleanPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStaticCleanPolyData
 * @brief   merge duplicate points, and/or remove unused points and/or remove degenerate cells
 *
 * vtkStaticCleanPolyData is a filter that takes polygonal data as input and
 * generates polygonal data as output. vtkStaticCleanPolyData will merge
 * duplicate points (within specified tolerance), and if enabled, transform
 * degenerate cells into appropriate forms (for example, a triangle is
 * converted into a line if two points of triangle are merged).
 *
 * Conversion of degenerate cells is controlled by the flags
 * ConvertLinesToPoints, ConvertPolysToLines, ConvertStripsToPolys which act
 * cumulatively such that a degenerate strip may become a poly.
 * The full set is
 * Line with 1 points -> Vert (if ConvertLinesToPoints)
 * Poly with 2 points -> Line (if ConvertPolysToLines)
 * Poly with 1 points -> Vert (if ConvertPolysToLines && ConvertLinesToPoints)
 * Strp with 3 points -> Poly (if ConvertStripsToPolys)
 * Strp with 2 points -> Line (if ConvertStripsToPolys && ConvertPolysToLines)
 * Strp with 1 points -> Vert (if ConvertStripsToPolys && ConvertPolysToLines
 *   && ConvertLinesToPoints)
 *
 * Internally this class uses vtkStaticPointLocator, which is a threaded, and
 * much faster locator than the incremental locators that vtkCleanPolyData
 * uses. Note because of these and other differences, the output of this
 * filter may be different than vtkCleanPolyData.
 *
 * Note that if you want to remove points that aren't used by any cells
 * (i.e., disable point merging), then use vtkCleanPolyData.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. The tolerance should be chosen carefully to avoid these problems.
 * Large tolerances (of size > locator bin width) may generate poor results.
 *
 * @warning
 * Merging close points with tolerance >0.0 is inherently an unstable problem
 * because the results are order dependent (e.g., the order in which points
 * are processed). When parallel computing, the order of processing points is
 * unpredictable, hence the results may vary between runs.
 *
 * @warning
 * If you wish to operate on a set of coordinates that has no cells, you must
 * add a vtkPolyVertex cell with all of the points to the PolyData (or use a
 * vtkVertexGlyphFilter) before using the vtkStaticCleanPolyData filter.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCleanPolyData
*/

#ifndef vtkStaticCleanPolyData_h
#define vtkStaticCleanPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkStaticPointLocator;

class VTKFILTERSCORE_EXPORT vtkStaticCleanPolyData : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print, and provide type information.
   */
  static vtkStaticCleanPolyData *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkStaticCleanPolyData,vtkPolyDataAlgorithm);
  //@}

  //@{
  /**
   * By default ToleranceIsAbsolute is false and Tolerance is
   * a fraction of Bounding box diagonal, if true, AbsoluteTolerance is
   * used when adding points to locator (merging)
   */
  vtkSetMacro(ToleranceIsAbsolute,vtkTypeBool);
  vtkBooleanMacro(ToleranceIsAbsolute,vtkTypeBool);
  vtkGetMacro(ToleranceIsAbsolute,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify tolerance in terms of fraction of bounding box length.  Default
   * is 0.0. This takes effect only if ToleranceIsAbsolute is false.
   */
  vtkSetClampMacro(Tolerance,double,0.0,1.0);
  vtkGetMacro(Tolerance,double);
  //@}

  //@{
  /**
   * Specify tolerance in absolute terms. Default is 1.0.
   */
  vtkSetClampMacro(AbsoluteTolerance,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(AbsoluteTolerance,double);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate lines to points. Default is On.
   */
  vtkSetMacro(ConvertLinesToPoints,vtkTypeBool);
  vtkBooleanMacro(ConvertLinesToPoints,vtkTypeBool);
  vtkGetMacro(ConvertLinesToPoints,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate polys to lines. Default is On.
   */
  vtkSetMacro(ConvertPolysToLines,vtkTypeBool);
  vtkBooleanMacro(ConvertPolysToLines,vtkTypeBool);
  vtkGetMacro(ConvertPolysToLines,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate strips to polys. Default is On.
   */
  vtkSetMacro(ConvertStripsToPolys,vtkTypeBool);
  vtkBooleanMacro(ConvertStripsToPolys,vtkTypeBool);
  vtkGetMacro(ConvertStripsToPolys,vtkTypeBool);
  //@}

  // This filter is difficult to stream.
  // To get invariant results, the whole input must be processed at once.
  // This flag allows the user to select whether strict piece invariance
  // is required.  By default it is on.  When off, the filter can stream,
  // but results may change.
  vtkSetMacro(PieceInvariant, vtkTypeBool);
  vtkGetMacro(PieceInvariant, vtkTypeBool);
  vtkBooleanMacro(PieceInvariant, vtkTypeBool);

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

  //@{
  /**
   * Retrieve the internal locator to manually configure it, for example
   * specifying the number of points per bucket. This method is generally
   * used for debugging or testing purposes.
   */
  vtkStaticPointLocator *GetLocator()
  { return this->Locator; }
  //@}

  /**
   * Get the MTime of this object also considering the locator.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkStaticCleanPolyData();
 ~vtkStaticCleanPolyData() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double Tolerance;
  double AbsoluteTolerance;
  vtkTypeBool ConvertLinesToPoints;
  vtkTypeBool ConvertPolysToLines;
  vtkTypeBool ConvertStripsToPolys;
  vtkTypeBool ToleranceIsAbsolute;
  vtkStaticPointLocator *Locator;

  vtkTypeBool PieceInvariant;
  int OutputPointsPrecision;

private:
  vtkStaticCleanPolyData(const vtkStaticCleanPolyData&) = delete;
  void operator=(const vtkStaticCleanPolyData&) = delete;
};

#endif
