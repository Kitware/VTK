/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.h
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
// .NAME vtkDiskSource - create a disk with hole in center
// .SECTION Description
// vtkDiskSource creates a polygonal disk with a hole in the center. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, and the radial and circumferential resolution of the 
// polygonal representation. 
// .SECTION SEE ALSO
// vtkLinearExtrusionFilter

#ifndef __vtkDiskSource_h
#define __vtkDiskSource_h

#include "vtkPolySource.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkDiskSource : public vtkPolySource 
{
public:
  vtkDiskSource();
  vtkDiskSource *New() {return new vtkDiskSource;};
  char *GetClassName() {return "vtkDiskSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify inner radius of hole in disc.
  vtkSetClampMacro(InnerRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(InnerRadius,float);

  // Description:
  // Specify outer radius of disc.
  vtkSetClampMacro(OuterRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(OuterRadius,float);

  // Description:
  // Set the number of points in radius direction.
  vtkSetClampMacro(RadialResolution,int,1,VTK_LARGE_INTEGER)
  vtkGetMacro(RadialResolution,int);

  // Description:
  // Set the number of points in circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,3,VTK_LARGE_INTEGER)
  vtkGetMacro(CircumferentialResolution,int);

protected:
  void Execute();
  float InnerRadius;
  float OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;

};

#endif


