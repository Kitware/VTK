/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToSphere.h
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
// .NAME vtkTextureMapToSphere - generate texture coordinates by mapping points to sphere
// .SECTION Description
// vtkTextureMapToSphere is a filter that generates 2D texture coordinates by
// mapping input dataset points onto a sphere. The sphere can either be user
// specified or generated automatically. (The sphere is generated
// automatically by computing the center (i.e., averaged coordinates) of the
// sphere.)  Note that the generated texture coordinates range between
// (0,1). The s-coordinate lies in the angular direction around the z-axis,
// measured counter-clockwise from the x-axis. The t-coordinate lies in the
// angular direction measured down from the north pole towards the south
// pole.
//
// A special ivar controls how the s-coordinate is generated. If PreventSeam
// is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
// to angles of 0->180 and 180->360).

// .SECTION Caveats
// The resulting texture coordinates will lie between (0,1), and the texture
// coordinates are determined with respect to the modeler's x-y-z coordinate
// system. Use the class vtkTransformTextureCoords to linearly scale and
// shift the origin of the texture coordinates (if necessary).

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToCylinder vtkTextureMapToBox
// vtkTransformTexture vtkThresholdTextureCoords

#ifndef __vtkTextureMapToSphere_h
#define __vtkTextureMapToSphere_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkTextureMapToSphere : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkTextureMapToSphere,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create object with Center (0,0,0) and the PreventSeam ivar is set to
  // true. The sphere center is automatically computed.
  static vtkTextureMapToSphere *New();

  // Description:
  // Specify a point defining the center of the sphere.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Turn on/off automatic sphere generation. This means it automatically
  // finds the sphere center.
  vtkSetMacro(AutomaticSphereGeneration,int);
  vtkGetMacro(AutomaticSphereGeneration,int);
  vtkBooleanMacro(AutomaticSphereGeneration,int);

  // Description:
  // Control how the texture coordinates are generated. If PreventSeam is
  // set, the s-coordinate ranges from 0->1 and 1->0 corresponding to the
  // theta angle variation between 0->180 and 180->0 degrees. Otherwise, the
  // s-coordinate ranges from 0->1 between 0->360 degrees.
  vtkSetMacro(PreventSeam,int);
  vtkGetMacro(PreventSeam,int);
  vtkBooleanMacro(PreventSeam,int);

protected:
  vtkTextureMapToSphere();
  ~vtkTextureMapToSphere() {};

  void Execute();

  float Center[3];
  int AutomaticSphereGeneration;
  int PreventSeam;

private:
  vtkTextureMapToSphere(const vtkTextureMapToSphere&);  // Not implemented.
  void operator=(const vtkTextureMapToSphere&);  // Not implemented.
};

#endif


