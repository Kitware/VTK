/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientShader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

// .NAME vtkEncodedGradientShader - 

// .SECTION see also
// 

#ifndef __vtkEncodedGradientShader_h
#define __vtkEncodedGradientShader_h

#include "vtkObject.h"

class vtkVolume;
class vtkRenderer;
class vtkEncodedGradientEstimator;

class VTK_EXPORT vtkEncodedGradientShader : public vtkObject
{
public:
  vtkEncodedGradientShader();
  ~vtkEncodedGradientShader();
  static vtkEncodedGradientShader *New() {return new vtkEncodedGradientShader;};
  const char *GetClassName() {return "vtkEncodedGradientShader";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Cause the shading table to be updated
  void UpdateShadingTable( vtkRenderer *ren, vtkVolume *vol,
			   vtkEncodedGradientEstimator *gradest);

  // Description:
  // Get the red/green/blue shading table.
  float *GetRedDiffuseShadingTable( void )    {return this->ShadingTable[0];};
  float *GetGreenDiffuseShadingTable( void )  {return this->ShadingTable[1];};
  float *GetBlueDiffuseShadingTable( void )   {return this->ShadingTable[2];};
  float *GetRedSpecularShadingTable( void )   {return this->ShadingTable[3];};
  float *GetGreenSpecularShadingTable( void ) {return this->ShadingTable[4];};
  float *GetBlueSpecularShadingTable( void )  {return this->ShadingTable[5];};

protected:

  // Description:
  // Build a shading table for a light with the specified direction,
  // and color for an object of the specified material properties.
  // material[0] = ambient, material[1] = diffuse, material[2] = specular
  // and material[3] = specular exponent.  If the update flag is 0,
  // the shading table is overwritten with these new shading values.
  // If the update_flag is 1, then the computed light contribution is
  // added to the current shading table values.
  void  BuildShadingTable( float light_direction[3],
			   float light_color[3],
			   float light_intensity,
			   float view_direction[3],
			   float material[4],
			   vtkEncodedGradientEstimator *gradest,
			   int update_flag );
  
  // The six shading tables (r diffuse ,g diffuse ,b diffuse, 
  // r specular, g specular, b specular ) - with an entry for each
  // encoded normal plus one entry at the end for the zero normal
  float                        *ShadingTable[6];

  int                          ShadingTableSize;

}; 


#endif
