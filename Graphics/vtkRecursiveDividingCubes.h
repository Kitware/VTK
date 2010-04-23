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
// .NAME vtkRecursiveDividingCubes - create points lying on isosurface (using recursive approach)
// .SECTION Description
// vtkRecursiveDividingCubes is a filter that generates points lying on a 
// surface of constant scalar value (i.e., an isosurface). Dense point 
// clouds (i.e., at screen resolution) will appear as a surface. Less dense 
// clouds can be used as a source to generate streamlines or to generate 
// "transparent" surfaces.
//
// This implementation differs from vtkDividingCubes in that it uses a 
// recursive procedure. In many cases this can result in generating 
// more points than the procedural implementation of vtkDividingCubes. This is
// because the recursive procedure divides voxels by multiples of powers of 
// two. This can over-constrain subdivision. One of the advantages of the 
// recursive technique is that the recursion is terminated earlier, which in
// some cases can be more efficient.

// .SECTION See Also
// vtkDividingCubes vtkContourFilter vtkMarchingCubes

#ifndef __vtkRecursiveDividingCubes_h
#define __vtkRecursiveDividingCubes_h

#include "vtkPolyDataAlgorithm.h"

class vtkVoxel;

class VTK_GRAPHICS_EXPORT vtkRecursiveDividingCubes : public vtkPolyDataAlgorithm
{
public:
  static vtkRecursiveDividingCubes *New();
  vtkTypeMacro(vtkRecursiveDividingCubes,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set isosurface value.
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);

  // Description:
  // Specify sub-voxel size at which to generate point.
  vtkSetClampMacro(Distance,double,1.0e-06,VTK_DOUBLE_MAX);
  vtkGetMacro(Distance,double);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  vtkRecursiveDividingCubes();
  ~vtkRecursiveDividingCubes();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  void SubDivide(double origin[3], double h[3], double values[8]);

  double Value;
  double Distance;
  int Increment;

  // working variable
  int Count;
  
  // to replace a static
  vtkVoxel *Voxel;
private:
  vtkRecursiveDividingCubes(const vtkRecursiveDividingCubes&); // Not implemented.
  void operator=(const vtkRecursiveDividingCubes&);  // Not implemented.
};

#endif
