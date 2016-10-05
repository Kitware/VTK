/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMarchingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMarchingCubes
 * @brief   generate isosurface(s) from volume/images
 *
 * vtkImageMarchingCubes is a filter that takes as input images (e.g., 3D
 * image region) and generates on output one or more isosurfaces.
 * One or more contour values must be specified to generate the isosurfaces.
 * Alternatively, you can specify a min/max scalar range and the number of
 * contours to generate a series of evenly spaced contour values.
 * This filter can stream, so that the entire volume need not be loaded at
 * once.  Streaming is controlled using the instance variable
 * InputMemoryLimit, which has units KBytes.
 *
 * @warning
 * This filter is specialized to volumes. If you are interested in
 * contouring other types of data, use the general vtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
 * @sa
 * vtkContourFilter vtkSliceCubes vtkMarchingSquares vtkSynchronizedTemplates3D
*/

#ifndef vtkImageMarchingCubes_h
#define vtkImageMarchingCubes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for direct access to ContourValues

class vtkCellArray;
class vtkFloatArray;
class vtkImageData;
class vtkPoints;

class VTKFILTERSGENERAL_EXPORT vtkImageMarchingCubes : public vtkPolyDataAlgorithm
{
public:
  static vtkImageMarchingCubes *New();
  vtkTypeMacro(vtkImageMarchingCubes,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Methods to set contour values
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  //@}

  /**
   * Because we delegate to vtkContourValues & refer to vtkImplicitFunction
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars, int);
  vtkGetMacro(ComputeScalars, int);
  vtkBooleanMacro(ComputeScalars, int);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly expensive
   * in both time and storage. If the output data will be processed by filters
   * that modify topology or geometry, it may be wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is fairly expensive
   * in both time and storage. Note that if ComputeNormals is on, gradients will
   * have to be calculated, but will not be stored in the output dataset.
   * If the output data will be processed by filters that modify topology or
   * geometry, it may be wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeGradients, int);
  vtkGetMacro(ComputeGradients, int);
  vtkBooleanMacro(ComputeGradients, int);
  //@}

  // Should be protected, but the templated functions need these
  int ComputeScalars;
  int ComputeNormals;
  int ComputeGradients;
  int NeedGradients;

  vtkCellArray *Triangles;
  vtkFloatArray *Scalars;
  vtkPoints *Points;
  vtkFloatArray *Normals;
  vtkFloatArray *Gradients;

  vtkIdType GetLocatorPoint(int cellX, int cellY, int edge);
  void AddLocatorPoint(int cellX, int cellY, int edge, vtkIdType ptId);
  void IncrementLocatorZ();

  //@{
  /**
   * The InputMemoryLimit determines the chunk size (the number of slices
   * requested at each iteration).  The units of this limit is KiloBytes.
   * For now, only the Z axis is split.
   */
  vtkSetMacro(InputMemoryLimit, vtkIdType);
  vtkGetMacro(InputMemoryLimit, vtkIdType);
  //@}

protected:
  vtkImageMarchingCubes();
  ~vtkImageMarchingCubes() VTK_OVERRIDE;

  int NumberOfSlicesPerChunk;
  vtkIdType InputMemoryLimit;

  vtkContourValues *ContourValues;

  vtkIdType *LocatorPointIds;
  int LocatorDimX;
  int LocatorDimY;
  int LocatorMinX;
  int LocatorMinY;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  void March(vtkImageData *inData, int chunkMin, int chunkMax,
             int numContours, double *values);
  void InitializeLocator(int min0, int max0, int min1, int max1);
  void DeleteLocator();
  vtkIdType *GetLocatorPointer(int cellX, int cellY, int edge);

private:
  vtkImageMarchingCubes(const vtkImageMarchingCubes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageMarchingCubes&) VTK_DELETE_FUNCTION;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkImageMarchingCubes::SetValue(int i, double value)
{this->ContourValues->SetValue(i,value);}

/**
 * Get the ith contour value.
 */
inline double vtkImageMarchingCubes::GetValue(int i)
{return this->ContourValues->GetValue(i);}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double *vtkImageMarchingCubes::GetValues()
{return this->ContourValues->GetValues();}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkImageMarchingCubes::GetValues(double *contourValues)
{this->ContourValues->GetValues(contourValues);}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkImageMarchingCubes::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

/**
 * Get the number of contours in the list of contour values.
 */
inline int vtkImageMarchingCubes::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkImageMarchingCubes::GenerateValues(int numContours, double range[2])
{this->ContourValues->GenerateValues(numContours, range);}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkImageMarchingCubes::GenerateValues(int numContours, double
                                                 rangeStart, double rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

#endif
