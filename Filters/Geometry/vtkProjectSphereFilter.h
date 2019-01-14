/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectSphereFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProjectSphereFilter
 * @brief   A filter to 'unroll' a sphere.  The
 * unroll longitude is -180.
 *
 *
*/

#ifndef vtkProjectSphereFilter_h
#define vtkProjectSphereFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkCell;
class vtkCellArray;
class vtkDataSetAttributes;
class vtkIdList;
class vtkIncrementalPointLocator;
class vtkUnstructuredGrid;

class VTKFILTERSGEOMETRY_EXPORT vtkProjectSphereFilter :
  public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkProjectSphereFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  static vtkProjectSphereFilter *New();

  //@{
  /**
   * Set the center of the sphere to be split. Default is 0,0,0.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  //@}

  //@{
  /**
   * Specify whether or not to keep the cells using a point at
   * a pole. The default is false.
   */
  vtkGetMacro(KeepPolePoints, bool);
  vtkSetMacro(KeepPolePoints, bool);
  vtkBooleanMacro(KeepPolePoints, bool);
  //@}

  //@{
  /**
   * Specify whether (true) or not to translate the points in the projected
   * transformation such that the input point with the smallest
   * radius is at 0. The default is false.
   */
  vtkGetMacro(TranslateZ, bool);
  vtkSetMacro(TranslateZ, bool);
  vtkBooleanMacro(TranslateZ, bool);
  //@}

protected:
  vtkProjectSphereFilter();
  ~vtkProjectSphereFilter() override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

  void TransformPointInformation(vtkPointSet* input, vtkPointSet* output, vtkIdList*);
  void TransformCellInformation(vtkPointSet* input, vtkPointSet* output, vtkIdList*);
  void TransformTensors(vtkIdType id, double* coord, vtkDataSetAttributes* arrays);

  /**
   * Parallel part of the algorithm to figure out the closest point
   * to the centerline (i.e. line connecting -90 latitude to 90 latitude)
   * if we don't build cells using points at the poles.
   */
  virtual void ComputePointsClosestToCenterLine(double, vtkIdList*)
  {}

  /**
   * If TranslateZ is true then this is the method that computes
   * the amount to translate.
   */
  virtual double GetZTranslation(vtkPointSet* input);

  /**
   * Split a cell into multiple cells because it stretches across the
   * SplitLongitude. splitSide is 1 for left side and 0 for sight side.
   */
  void SplitCell(  vtkPointSet* input, vtkPointSet* output, vtkIdType inputCellId,
                   vtkIncrementalPointLocator* locator, vtkCellArray* connectivity,
                   int splitSide);

  void SetCellInformation(
    vtkUnstructuredGrid* output, vtkCell* cell, vtkIdType numberOfNewCells);

private:
  vtkProjectSphereFilter(const vtkProjectSphereFilter &) = delete;
  void operator=(const vtkProjectSphereFilter &) = delete;

  double Center[3];
  const double SplitLongitude;
  bool KeepPolePoints;
  bool TranslateZ;
};

#endif // vtkProjectSphereFilter_h
