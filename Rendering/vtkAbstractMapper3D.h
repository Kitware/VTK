/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkAbstractMapper3D - abstract class specifies interface to map 3D data
// .SECTION Description
// vtkAbstractMapper3D is an abstract class to specify interface between 3D
// data and graphics primitives or software rendering techniques. Subclasses
// of vtkAbstractMapper3D can be used for rendering geometry or rendering
// volumetric data.
//
// This class also defines an API to support hardware clipping planes (at most
// six planes can be defined). It also provides geometric data about the input
// data it maps, such as the bounding box and center.
//
// .SECTION See Also
// vtkAbstractMapper vtkMapper vtkPolyDataMapper vtkVolumeMapper

#ifndef __vtkAbstractMapper3D_h
#define __vtkAbstractMapper3D_h

#include "vtkAbstractMapper.h"

class vtkWindow;
class vtkDataSet;

class VTK_EXPORT vtkAbstractMapper3D : public vtkAbstractMapper
{
public:
  vtkTypeMacro(vtkAbstractMapper3D,vtkAbstractMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds()=0;

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual void GetBounds(float bounds[6]);
  
  // Description:
  // Return the Center of this mapper's data.
  float *GetCenter();
  
  // Description:
  // Return the diagonal length of this mappers bounding box.
  float GetLength();

  // Description:
  // Is this a ray cast mapper? A subclass would return 1 if the
  // ray caster is needed to generate an image from this mapper.
  virtual int IsARayCastMapper() {return 0;};

  // Description:
  // Is this a "render into image" mapper? A subclass would return 1 if the
  // mapper produces an image by rendering into a software image buffer.
  virtual int IsARenderIntoImageMapper() {return 0;};

  // Description:
  // Update the network connected to this mapper.
  virtual void Update()=0;

protected:
   vtkAbstractMapper3D();
  ~vtkAbstractMapper3D() {};
  vtkAbstractMapper3D(const vtkAbstractMapper3D&);
  void operator=(const vtkAbstractMapper3D&);

  float Bounds[6];
  float Center[3];

};

#endif
