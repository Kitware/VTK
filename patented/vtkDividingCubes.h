/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDividingCubes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,719,585
    "Dividing Cubes System and Method for the Display of Surface Structures
    Contained Within the Interior Region of a Solid Body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Mike Silver
        GE Medical Systems
        16705 West Lincoln Ave., 
        NB 900
        New Berlin, WI, 53151
        Phone:1-414-827-3400 
    for more information.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkDividingCubes - create points lying on isosurface
// .SECTION Description
// vtkDividingCubes is a filter that generates points lying on a surface
// of constant scalar value (i.e., an isosurface). Dense point clouds (i.e.,
// at screen resolution) will appear as a surface. Less dense clouds can be 
// used as a source to generate streamlines or to generate "transparent"
// surfaces. This filter is based on the generate program written by
// H. Cline, S. Ludke and W. Lorensen.
//
// The density of the point cloud is controlled by the Distance instance 
// variable. This is a distance value in global coordinates specifying the 
// approximate distance between points.

#ifndef __vtkDividingCubes_h
#define __vtkDividingCubes_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_EXPORT vtkDividingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkDividingCubes();
  static vtkDividingCubes *New() {return new vtkDividingCubes;};
  const char *GetClassName() {return "vtkDividingCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set isosurface value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify sub-voxel size at which to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,VTK_LARGE_FLOAT);
  vtkGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  void Execute();
  void SubDivide(float origin[3], int dim[3], float h[3], float values[8]);

  float Value;
  float Distance;
  int Increment;

  // working variable
  int Count;
};

#endif


