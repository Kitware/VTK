/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSynchronizedTemplates3D
 * @brief   generate isosurface from structured points
 *
 * vtkSynchronizedTemplates3D is a 3D implementation of the synchronized
 * template algorithm. Note that vtkContourFilter will automatically
 * use this class when appropriate.
 *
 * @warning
 * This filter is specialized to 3D images (aka volumes).
 *
 * @sa
 * vtkContourFilter vtkFlyingEdges3D vtkMarchingCubes
 * vtkSynchronizedTemplates2D vtkDiscreteFlyingEdges3D
*/

#ifndef vtkSynchronizedTemplates3D_h
#define vtkSynchronizedTemplates3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Passes calls through

class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkSynchronizedTemplates3D : public vtkPolyDataAlgorithm
{
public:
  static vtkSynchronizedTemplates3D *New();

  vtkTypeMacro(vtkSynchronizedTemplates3D,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Because we delegate to vtkContourValues
   */
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
  * If this is enabled (by default), the output will be triangles
  * otherwise, the output will be the intersection polygons
  */
  vtkSetMacro(GenerateTriangles,vtkTypeBool);
  vtkGetMacro(GenerateTriangles,vtkTypeBool);
  vtkBooleanMacro(GenerateTriangles,vtkTypeBool);
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

  void ThreadedExecute(vtkImageData *data,
                       vtkInformation *inInfo,
                       vtkInformation *outInfo,
                       vtkDataArray *inScalars);

  //@{
  /**
   * Determines the chunk size for streaming.  This filter will act like a
   * collector: ask for many input pieces, but generate one output.  Limit is
   * in KBytes
   */
  void SetInputMemoryLimit(unsigned long limit);
  unsigned long GetInputMemoryLimit();
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  //@}

protected:
  vtkSynchronizedTemplates3D();
  ~vtkSynchronizedTemplates3D() override;

  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkContourValues *ContourValues;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  int ArrayComponent;

  vtkTypeBool GenerateTriangles;

private:
  vtkSynchronizedTemplates3D(const vtkSynchronizedTemplates3D&) = delete;
  void operator=(const vtkSynchronizedTemplates3D&) = delete;
};


// template table.

extern int VTKFILTERSCORE_EXPORT VTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_1[];
extern int VTKFILTERSCORE_EXPORT VTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_2[];

#endif
