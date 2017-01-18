/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericContourFilter
 * @brief   generate isocontours from input dataset
 *
 * vtkGenericContourFilter is a filter that takes as input any (generic)
 * dataset and generates on output isosurfaces and/or isolines. The exact
 * form of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data consisting of
 * 2D cells will generate isolines, and data with 1D or 0D cells will
 * generate isopoints. Combinations of output type are possible if the input
 * dimension is mixed.
 *
 * To use this filter you must specify one or more contour values.
 * You can either use the method SetValue() to specify each contour
 * value, or use GenerateValues() to generate a series of evenly
 * spaced contours. You can use ComputeNormalsOn to compute the normals
 * without the need of a vtkPolyDataNormals
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical vtkDataSet (and subclasses). vtkGenericDataSet is a more
 * complex cousin of vtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * vtkContourFilter vtkGenericDataSet
*/

#ifndef vtkGenericContourFilter_h
#define vtkGenericContourFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkContourValues;
class vtkIncrementalPointLocator;
class vtkPointData;
class vtkCellData;

class VTKFILTERSGENERIC_EXPORT vtkGenericContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGenericContourFilter,
                       vtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static vtkGenericContourFilter *New();

  typedef double PointType[3];  // Arbitrary definition of a point

  //@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, float value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  //@}

  /**
   * Modified GetMTime Because we delegate to vtkContourValues
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is
   * fairly expensive in both time and storage. Note that if
   * ComputeNormals is on, gradients will have to be calculated, but
   * will not be stored in the output dataset.  If the output data
   * will be processed by filters that modify topology or geometry, it
   * may be wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);
  //@}

  //@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars,int);
  vtkGetMacro(ComputeScalars,int);
  vtkBooleanMacro(ComputeScalars,int);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  //@{
  /**
   * If you want to contour by an arbitrary scalar attribute, then set its
   * name here.
   * By default this in NULL and the filter will use the active scalar array.
   */
  vtkGetStringMacro(InputScalarsSelection);
  virtual void SelectInputScalars(const char *fieldName);
  //@}

protected:
  vtkGenericContourFilter();
  ~vtkGenericContourFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  vtkContourValues *ContourValues;
  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkIncrementalPointLocator *Locator;

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  // Used internal by vtkGenericAdaptorCell::Contour()
  vtkPointData *InternalPD;
  vtkPointData *SecondaryPD;
  vtkCellData  *SecondaryCD;

private:
  vtkGenericContourFilter(const vtkGenericContourFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericContourFilter&) VTK_DELETE_FUNCTION;
};
#endif
