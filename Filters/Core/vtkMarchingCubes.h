/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMarchingCubes
 * @brief   generate isosurface(s) from volume
 *
 * vtkMarchingCubes is a filter that takes as input a volume (e.g., 3D
 * structured point set) and generates on output one or more isosurfaces.
 * One or more contour values must be specified to generate the isosurfaces.
 * Alternatively, you can specify a min/max scalar range and the number of
 * contours to generate a series of evenly spaced contour values.
 *
 * @warning
 * This filter is specialized to volumes. If you are interested in
 * contouring other types of data, use the general vtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
 *
 * @sa
 * Much faster implementations for isocontouring are available. In
 * particular, vtkFlyingEdges3D and vtkFlyingEdges2D are much faster
 * and if built with the right options, multithreaded, and scale well
 * with additional processors.
 *
 * @sa
 * If you are interested in extracting surfaces from label maps,
 * consider using vtkDiscreteFlyingEdges3D, vtkDiscreteFlyingEdges2D, or
 * vtkDiscreteMarchingCubes.
 *
 * @sa
 * vtkFlyingEdges3D vtkFlyingEdges2D vtkSynchronizedTemplates3D
 * vtkSynchronizedTemplates2D vtkContourFilter vtkSliceCubes
 * vtkMarchingSquares vtkDividingCubes vtkDiscreteMarchingCubes
*/

#ifndef vtkMarchingCubes_h
#define vtkMarchingCubes_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for direct access to ContourValues

class vtkIncrementalPointLocator;

class VTKFILTERSCORE_EXPORT vtkMarchingCubes : public vtkPolyDataAlgorithm
{
public:
  static vtkMarchingCubes *New();
  vtkTypeMacro(vtkMarchingCubes,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Methods to set contour values
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);

  // Because we delegate to vtkContourValues
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals,vtkTypeBool);
  vtkGetMacro(ComputeNormals,vtkTypeBool);
  vtkBooleanMacro(ComputeNormals,vtkTypeBool);
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
  vtkSetMacro(ComputeGradients,vtkTypeBool);
  vtkGetMacro(ComputeGradients,vtkTypeBool);
  vtkBooleanMacro(ComputeGradients,vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars,vtkTypeBool);
  vtkGetMacro(ComputeScalars,vtkTypeBool);
  vtkBooleanMacro(ComputeScalars,vtkTypeBool);
  //@}

  //@{
  /**
   * override the default locator.  Useful for changing the number of
   * bins for performance or specifying a more aggressive locator.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  vtkMarchingCubes();
  ~vtkMarchingCubes() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  vtkContourValues *ContourValues;
  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkIncrementalPointLocator *Locator;
private:
  vtkMarchingCubes(const vtkMarchingCubes&) = delete;
  void operator=(const vtkMarchingCubes&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkMarchingCubes::SetValue(int i, double value)
{this->ContourValues->SetValue(i,value);}

/**
 * Get the ith contour value.
 */
inline double vtkMarchingCubes::GetValue(int i)
{return this->ContourValues->GetValue(i);}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double *vtkMarchingCubes::GetValues()
{return this->ContourValues->GetValues();}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkMarchingCubes::GetValues(double *contourValues)
{this->ContourValues->GetValues(contourValues);}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkMarchingCubes::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

/**
 * Get the number of contours in the list of contour values.
 */
inline int vtkMarchingCubes::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkMarchingCubes::GenerateValues(int numContours, double range[2])
{this->ContourValues->GenerateValues(numContours, range);}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkMarchingCubes::GenerateValues(int numContours, double
                                             rangeStart, double rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

#endif
