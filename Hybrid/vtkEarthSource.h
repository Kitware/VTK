/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEarthSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tom Johnson at Johnson Scientific International who
             developed and contributed this class. Also see
             ftp://www.ou.edu/pub/simtelnet/msdos/worldmap and the
             data by John B. Allison for more information.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkEarthSource - create the continents of the Earth as a sphere
// .SECTION Description
// vtkEarthSource creates a spherical rendering of the geographical shapes
// of the major continents of the earth. The OnRatio determines
// how much of the data is actually used. The radius defines the radius
// of the sphere at which the continents are placed. Obtains data from
// an imbedded array of coordinates.

#ifndef __vtkEarthSource_h
#define __vtkEarthSource_h

#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkEarthSource : public vtkPolyDataSource 
{
public:
  static vtkEarthSource *New();
  vtkTypeMacro(vtkEarthSource,vtkPolyDataSource);
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
  vtkEarthSource();
  ~vtkEarthSource() {};
  vtkEarthSource(const vtkEarthSource&);
  void operator=(const vtkEarthSource&);

  void Execute();

  float Radius;
  int OnRatio;
  int Outline;
};

#endif










