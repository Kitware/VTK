/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearSynchronizedTemplates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearSynchronizedTemplates
 * @brief   generate isosurface from rectilinear grid
 *
 *
 * vtkRectilinearSynchronizedTemplates is a 3D implementation (for rectilinear
 * grids) of the synchronized template algorithm. Note that vtkContourFilter
 * will automatically use this class when appropriate.
 *
 * @warning
 * This filter is specialized to rectilinear grids.
 *
 * @sa
 * vtkContourFilter vtkSynchronizedTemplates2D vtkSynchronizedTemplates3D
*/

#ifndef vtkRectilinearSynchronizedTemplates_h
#define vtkRectilinearSynchronizedTemplates_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Passes calls through

class vtkRectilinearGrid;
class vtkDataArray;

class VTKFILTERSCORE_EXPORT vtkRectilinearSynchronizedTemplates : public vtkPolyDataAlgorithm
{
public:
  static vtkRectilinearSynchronizedTemplates *New();

  vtkTypeMacro(vtkRectilinearSynchronizedTemplates,vtkPolyDataAlgorithm);
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
   * Compute the spacing between this point and its 6 neighbors.  This method
   * needs to be public so it can be accessed from a templated function.
   */
  void ComputeSpacing(vtkRectilinearGrid *data, int i, int j, int k,
                      int extent[6], double spacing[6]);

protected:
  vtkRectilinearSynchronizedTemplates();
  ~vtkRectilinearSynchronizedTemplates() override;

  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkTypeBool GenerateTriangles;

  vtkContourValues *ContourValues;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  int ArrayComponent;

  void* GetScalarsForExtent(vtkDataArray *array, int extent[6],
                            vtkRectilinearGrid *input);

private:
  vtkRectilinearSynchronizedTemplates(const vtkRectilinearSynchronizedTemplates&) = delete;
  void operator=(const vtkRectilinearSynchronizedTemplates&) = delete;
};

// template table.

extern int VTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_1[];
extern int VTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_2[];

#endif
