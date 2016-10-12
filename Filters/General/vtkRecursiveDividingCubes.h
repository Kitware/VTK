/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveDividingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRecursiveDividingCubes
 * @brief   create points laying on isosurface (using recursive approach)
 *
 * vtkRecursiveDividingCubes is a filter that generates points laying on a
 * surface of constant scalar value (i.e., an isosurface). Dense point
 * clouds (i.e., at screen resolution) will appear as a surface. Less dense
 * clouds can be used as a source to generate streamlines or to generate
 * "transparent" surfaces.
 *
 * This implementation differs from vtkDividingCubes in that it uses a
 * recursive procedure. In many cases this can result in generating
 * more points than the procedural implementation of vtkDividingCubes. This is
 * because the recursive procedure divides voxels by multiples of powers of
 * two. This can over-constrain subdivision. One of the advantages of the
 * recursive technique is that the recursion is terminated earlier, which in
 * some cases can be more efficient.
 *
 * @sa
 * vtkDividingCubes vtkContourFilter vtkMarchingCubes
*/

#ifndef vtkRecursiveDividingCubes_h
#define vtkRecursiveDividingCubes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkVoxel;

class VTKFILTERSGENERAL_EXPORT vtkRecursiveDividingCubes : public vtkPolyDataAlgorithm
{
public:
  static vtkRecursiveDividingCubes *New();
  vtkTypeMacro(vtkRecursiveDividingCubes,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set isosurface value.
   */
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);
  //@}

  //@{
  /**
   * Specify sub-voxel size at which to generate point.
   */
  vtkSetClampMacro(Distance,double,1.0e-06,VTK_DOUBLE_MAX);
  vtkGetMacro(Distance,double);
  //@}

  //@{
  /**
   * Every "Increment" point is added to the list of points. This parameter, if
   * set to a large value, can be used to limit the number of points while
   * retaining good accuracy.
   */
  vtkSetClampMacro(Increment,int,1,VTK_INT_MAX);
  vtkGetMacro(Increment,int);
  //@}

protected:
  vtkRecursiveDividingCubes();
  ~vtkRecursiveDividingCubes() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  void SubDivide(double origin[3], double h[3], double values[8]);

  double Value;
  double Distance;
  int Increment;

  // working variable
  int Count;

  // to replace a static
  vtkVoxel *Voxel;
private:
  vtkRecursiveDividingCubes(const vtkRecursiveDividingCubes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRecursiveDividingCubes&) VTK_DELETE_FUNCTION;
};

#endif
