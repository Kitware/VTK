/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToBox.h
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
// .NAME vtkTextureMapToBox - generate 3D texture coordinates by mapping points into bounding box
// .SECTION Description
// vtkTextureMapToBox is a filter that generates 3D texture coordinates
// by mapping input dataset points onto a bounding box. The bounding box
// can either be user specified or generated automatically. If the box
// is generated automatically, all points will lie inside of it. If a
// point lies outside the bounding box (only for manual box 
// specification), its generated texture coordinate will be mapped
// into the r-s-t texture coordinate range.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToCylinder vtkTextureMapToSphere
// vtkThresholdTextureCoords

#ifndef __vtkTextureMapToBox_h
#define __vtkTextureMapToBox_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkTextureMapToBox : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkTextureMapToBox,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with r-s-t range=(0,1) and automatic box generation turned on.
  static vtkTextureMapToBox *New();
  
  // Description:
  // Specify the bounding box to map into.
  void SetBox(float xmin, float xmax, float ymin, float ymax, 
	      float zmin, float zmax);
  void SetBox(float *box);
  vtkGetVectorMacro(Box,float,6);

  // Description:
  // Specify r-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(RRange,float);
  vtkGetVectorMacro(RRange,float,2);

  // Description:
  // Specify s-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic bounding box generation.
  vtkSetMacro(AutomaticBoxGeneration,int);
  vtkGetMacro(AutomaticBoxGeneration,int);
  vtkBooleanMacro(AutomaticBoxGeneration,int);

protected:
  vtkTextureMapToBox();
  ~vtkTextureMapToBox() {};
  vtkTextureMapToBox(const vtkTextureMapToBox&);
  void operator=(const vtkTextureMapToBox&);

  void Execute();
  float Box[6];
  float RRange[2];
  float SRange[2];
  float TRange[2];
  int AutomaticBoxGeneration;
};

#endif


