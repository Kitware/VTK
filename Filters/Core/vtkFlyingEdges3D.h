/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlyingEdges3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFlyingEdges3D
 * @brief   generate isosurface from 3D image data (volume)
 *
 * vtkFlyingEdges3D is a reference implementation of the 3D version of the
 * flying edges algorithm. It is designed to be highly scalable (i.e.,
 * parallelizable) for large data. It implements certain performance
 * optimizations including computational trimming to rapidly eliminate
 * processing of data regions, packed bit representation of case table
 * values, single edge intersection, elimination of point merging, and
 * elimination of any reallocs (due to dynamic data insertion). Note that
 * computational trimming is a method to reduce total computational cost in
 * which partial computational results can be used to eliminate future
 * computations.
 *
 * This is a four-pass algorithm. The first pass processes all x-edges and
 * builds x-edge case values (which, when the four x-edges defining a voxel
 * are combined, are equivalent to vertex-based case table except edge-based
 * approaches are separable in support of parallel computing). Next x-voxel
 * rows are processed to gather information from yz-edges (basically to count
 * the number of y-z edge intersections and triangles generated). In the third
 * pass a prefix sum is used to count and allocate memory for the output
 * primitives. Finally in the fourth pass output primitives are generated into
 * pre-allocated arrays. This implementation uses voxel cell axes (a x-y-z
 * triad located at the voxel origin) to ensure that each edge is intersected
 * at most one time. Note that this implementation also reuses the VTK
 * Marching Cubes case table, although the vertex-based MC table is
 * transformed into an edge-based table on object instantiation.
 *
 * See the paper "Flying Edges: A High-Performance Scalable Isocontouring
 * Algorithm" by Schroeder, Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 3D volumes. This implementation can produce
 * degenerate triangles (i.e., zero-area triangles).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkContourFilter vtkFlyingEdges2D vtkSynchronizedTemplates3D
 * vtkMarchingCubes vtkSMPFlyingEdges3D
*/

#ifndef vtkFlyingEdges3D_h
#define vtkFlyingEdges3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Passes calls through

class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkFlyingEdges3D : public vtkPolyDataAlgorithm
{
public:
  static vtkFlyingEdges3D *New();
  vtkTypeMacro(vtkFlyingEdges3D,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be processed
   * by filters that modify topology or geometry, it may be wise to turn
   * Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is fairly
   * expensive in both time and storage. Note that if ComputeNormals is on,
   * gradients will have to be calculated, but will not be stored in the
   * output dataset. If the output data will be processed by filters that
   * modify topology or geometry, it may be wise to turn Normals and
   * Gradients off.
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
   * Indicate whether to interpolate other attribute data. That is, as the
   * isosurface is generated, interpolate all point attribute data across
   * the edge. This is independent of scalar interpolation, which is
   * controlled by the ComputeScalars flag.
   */
  vtkSetMacro(InterpolateAttributes,int);
  vtkGetMacro(InterpolateAttributes,int);
  vtkBooleanMacro(InterpolateAttributes,int);
  //@}

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value) {this->ContourValues->SetValue(i,value);}

  /**
   * Get the ith contour value.
   */
  double GetValue(int i) {return this->ContourValues->GetValue(i);}

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double *GetValues() {return this->ContourValues->GetValues();}

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double *contourValues) {
    this->ContourValues->GetValues(contourValues);}

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number) {
    this->ContourValues->SetNumberOfContours(number);}

  /**
   * Get the number of contours in the list of contour values.
   */
  int GetNumberOfContours() {
    return this->ContourValues->GetNumberOfContours();}

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2]) {
    this->ContourValues->GenerateValues(numContours, range);}

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  //@}

protected:
  vtkFlyingEdges3D();
  ~vtkFlyingEdges3D() VTK_OVERRIDE;

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  int InterpolateAttributes;
  int ArrayComponent;
  vtkContourValues *ContourValues;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkFlyingEdges3D(const vtkFlyingEdges3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFlyingEdges3D&) VTK_DELETE_FUNCTION;
};

#endif
