/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.h
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

#ifndef __vtkVolumeRayCastFunction_h
#define __vtkVolumeRayCastFunction_h

#include "vtkObject.h"

class vtkRenderer;
class vtkVolume;
class vtkVolumeRayCastMapper;

class VTK_EXPORT vtkVolumeRayCastFunction : public vtkObject
{
public:
  const char *GetClassName() {return "vtkVolumeRayCastFunction";};

  // Description:
  // Do the basic initialization. This includes saving the parameters
  // passed in into local variables, as well as grabbing some useful
  // info from the volume property and normal encoder. This initialize
  // routine is called once per render. It also calls the 
  // SpecificFunctionInitialize of the subclass function.
  void FunctionInitialize( vtkRenderer *ren,
			   vtkVolume   *vol,
			   vtkVolumeRayCastMapper *mapper,
			   float *opacity_tf_array,
			   float *rgb_tf_array,
			   float *gray_tf_array,
			   int   tf_array_size);

  // Description:
  // Give a ray type (0 = unsigned char, 1 = unsigned short,
  // 2 = short) cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final compositing value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha
  // pixel_value[4] = depth, and pixel_value[5] = number of steps
  virtual void CastARay( int ray_type, void *data_ptr,
		 float ray_position[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] )=0;

  virtual float GetZeroOpacityThreshold( vtkVolume *vol )=0;

  // Description:
  // These are some variables set during FunctionInitialize. They
  // are either passed into that function, or acquired using Get 
  // methods and saved locally for performance reasons. They are
  // public because they need to be accessed by a templated method
  // which is not a member method.
  float                        *RGBTFArray;
  float                        *GrayTFArray;
  float                        *OpacityTFArray;
  int                          TFArraySize;
  int                          Shading;
  int                          ColorChannels;
  int                          InterpolationType;
  int                          DataIncrement[3];
  int                          DataSize[3];
  float                        *RedDiffuseShadingTable;
  float                        *GreenDiffuseShadingTable;
  float                        *BlueDiffuseShadingTable;
  float                        *RedSpecularShadingTable;
  float                        *GreenSpecularShadingTable;
  float                        *BlueSpecularShadingTable;
  unsigned short               *EncodedNormals;

protected:

  // Description:
  // This method gives the subclass a chance to do any special
  // initialization that it may need to do
  virtual void SpecificFunctionInitialize( vtkRenderer *ren,
					   vtkVolume   *vol,
					   vtkVolumeRayCastMapper *mapper )=0;
};

#endif
