/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DCubes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
// .NAME vtkDividingCubes - create points lying on iso-surface
// .SECTION Description
// vtkDividingCubes is a filter that generates points laying on a surface
// of constant scalar value (i.e., an iso-surface). Dense point clouds (i.e.,
// at screen resolution) will appear as a surface. Less dense clouds can be 
// used as a source to generate streamlines or to generate "transparent"
// surfaces.

#ifndef __vtkDividingCubes_h
#define __vtkDividingCubes_h

#include "SPt2Poly.hh"

class vtkDividingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkDividingCubes();
  ~vtkDividingCubes() {};
  char *GetClassName() {return "vtkDividingCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set iso-surface value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify sub-voxel size at which to generate point..
  vtkSetClampMacro(Distance,float,1.0e-06,LARGE_FLOAT);
  vtkGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  void Execute();
  void SubDivide(float origin[3], float h[3], float values[8]);
  void AddPoint(float x[3]);

  float Value;
  float Distance;
  int Increment;

  // wworking variables
  int Count;
};

#endif


