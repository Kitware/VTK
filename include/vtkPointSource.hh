/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSource.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPointSource - create a random cloud of points
// .SECTION Description
// vtkPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// The location of the points is random within the sphere.

#ifndef __vtkPointSource_h
#define __vtkPointSource_h

#include "vtkPolySource.hh"

class vtkPointSource : public vtkPolySource 
{
public:
  vtkPointSource(int numPts=10);
  char *GetClassName() {return "vtkPointSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPoints,int);

  // Description:
  // Set the center of the point cloud.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the radius of the point cloud.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

protected:
  void Execute();
  int NumberOfPoints;
  float Center[3];
  float Radius;
};

#endif


