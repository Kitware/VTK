/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientShader.h
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

// .NAME vtkEncodedGradientShader - Compute shading tables for encoded normals.
//
// .SECTION Description
// vtkEncodedGradientShader computes shading tables for encoded normals 
// that indicates the amount of diffuse and specular illumination that is 
// received from all light sources at a surface location with that normal.
// For diffuse illumination this is accurate, but for specular illumination
// it is approximate for perspective projections since the center view
// direction is always used as the view direction. Since the shading table is
// dependent on the volume (for the transformation that must be applied to
// the normals to put them into world coordinates) there is a shading table
// per volume. This is necessary because multiple volumes can share a 
// volume mapper.

#ifndef __vtkEncodedGradientShader_h
#define __vtkEncodedGradientShader_h

#include "vtkObject.h"

class vtkVolume;
class vtkRenderer;
class vtkEncodedGradientEstimator;

#define VTK_MAX_SHADING_TABLES   100

class VTK_RENDERING_EXPORT vtkEncodedGradientShader : public vtkObject
{
public:
  static vtkEncodedGradientShader *New();
  vtkTypeMacro(vtkEncodedGradientShader,vtkObject);

  // Description:
  // Print the vtkEncodedGradientShader
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set / Get the intensity diffuse / specular light used for the
  // zero normals. 
  vtkSetClampMacro( ZeroNormalDiffuseIntensity,  float, 0.0, 1.0);
  vtkGetMacro( ZeroNormalDiffuseIntensity, float );
  vtkSetClampMacro( ZeroNormalSpecularIntensity, float, 0.0, 1.0);
  vtkGetMacro( ZeroNormalSpecularIntensity, float );

  // Description:
  // Cause the shading table to be updated
  void UpdateShadingTable( vtkRenderer *ren, vtkVolume *vol,
			   vtkEncodedGradientEstimator *gradest);

  // Description:
  // Get the red/green/blue shading table.
  float *GetRedDiffuseShadingTable(    vtkVolume *vol );
  float *GetGreenDiffuseShadingTable(  vtkVolume *vol );
  float *GetBlueDiffuseShadingTable(   vtkVolume *vol );
  float *GetRedSpecularShadingTable(   vtkVolume *vol );
  float *GetGreenSpecularShadingTable( vtkVolume *vol );
  float *GetBlueSpecularShadingTable(  vtkVolume *vol );

protected:
  vtkEncodedGradientShader();
  ~vtkEncodedGradientShader();
  vtkEncodedGradientShader(const vtkEncodedGradientShader&);
  void operator=(const vtkEncodedGradientShader&);

  // Description:
  // Build a shading table for a light with the specified direction,
  // and color for an object of the specified material properties.
  // material[0] = ambient, material[1] = diffuse, material[2] = specular
  // and material[3] = specular exponent.  If the ambient flag is 1, 
  // then ambient illumination is added. If not, then this means we 
  // are calculating the "other side" of two sided lighting, so no 
  // ambient intensity is added in. If the update flag is 0,
  // the shading table is overwritten with these new shading values.
  // If the updateFlag is 1, then the computed light contribution is
  // added to the current shading table values. There is one shading
  // table per volume, and the index value indicated which index table
  // should be used. It is computed in the UpdateShadingTable method.
  void  BuildShadingTable( int index,
			   float lightDirection[3],
			   float lightColor[3],
			   float lightIntensity,
			   float viewDirection[3],
			   float material[4],
			   int twoSided,
			   vtkEncodedGradientEstimator *gradest,
			   int updateFlag );
  
  // The six shading tables (r diffuse ,g diffuse ,b diffuse, 
  // r specular, g specular, b specular ) - with an entry for each
  // encoded normal plus one entry at the end for the zero normal
  // There is one shading table per volume listed in the ShadingTableVolume
  // array. A null entry indicates an available slot.
  float                        *ShadingTable[VTK_MAX_SHADING_TABLES][6];
  vtkVolume                    *ShadingTableVolume[VTK_MAX_SHADING_TABLES];
  int                          ShadingTableSize[VTK_MAX_SHADING_TABLES];

  // The intensity of light used for the zero normals, since it
  // can not be computed from the normal angles. Defaults to 0.0.
  float    ZeroNormalDiffuseIntensity;
  float    ZeroNormalSpecularIntensity;
}; 


#endif
