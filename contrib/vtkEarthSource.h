/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEarthSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tom Johnson at Johnson Scientific International who
             developed and contributed this class.

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
// .NAME vtkEarthSource - create the continents of the Earth as a sphere
// .SECTION Description
// vtkEarthSource creates a spherical rendering of the geographical shapes
// of the major continents of the earth. The OnRatio determines
// how much of the data is actually used. The radius defines the radius
// of the sphere at which the continents are placed. Obtains data from
// an imbeded array of coordinates.

#ifndef __vtkEarthSource_h
#define __vtkEarthSource_h

#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkEarthSource : public vtkPolyDataSource 
{
public:
  vtkEarthSource();
  static vtkEarthSource *New() {return new vtkEarthSource;};
  const char *GetClassName() {return "vtkEarthSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set radius of earth.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on every nth entity. This controls how much detail the model
  // will have. The maximum ratio is sixteen. (The smaller OnRatio, the more
  // detail there is.)
  vtkSetClampMacro(OnRatio,int,1,16);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Turn on/off drawing continents as filled polygons or as wireframe outlines.
  // Warning: some graphics systems will have trouble with the very large, concave 
  // filled polygons. Recommend you use OutlienOn (i.e., disable filled polygons) 
  // for now.
  vtkSetMacro(Outline,int);
  vtkGetMacro(Outline,int);
  vtkBooleanMacro(Outline,int);

protected:
  void Execute();

  float Radius;
  int OnRatio;
  int Outline;
};

#endif










