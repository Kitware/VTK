/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeVolumeRayCaster.h
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
// .NAME vtkCompositeVolumeRayCaster - Creates composite projections using Depth Parc acceleration
// .SECTION Description
// vtkCompositeVolumeRayCaster is a concrete implementation of vtkDepthPARCMapper
// It can be used to create a composite projection of scalar data using
// and alpha / 1 - alpha compositing scheme.  Shading can be on or off.
// Color transfer functions are used to define the color of the data,
// a 1d transfer function is used to define the opacity of each scalar
// value, and if shading is on, another 1d transfer function is used to
// map gradient magnitudes to opacity.

// .SECTION see also
// vtkDepthPARCMapper

#ifndef __vtkCompositeVolumeRayCaster_h
#define __vtkCompositeVolumeRayCaster_h

#include "vtkVolumeRayCaster.h"
#include "vtkVolume.h"
#include "vtkPiecewiseFunction.h"
#include "vtkNormalEncoder.h"
#include "vtkPolyData.h"
#include "vtkColorTransferFunction.h"

#define VTK_SINGLE_COLOR 0
#define VTK_TRANSFER_FUNCTION 1

class VTK_EXPORT vtkCompositeVolumeRayCaster : public vtkVolumeRayCaster
{
public:
  vtkCompositeVolumeRayCaster();
  ~vtkCompositeVolumeRayCaster();
  static vtkCompositeVolumeRayCaster *New() {return new vtkCompositeVolumeRayCaster;};
  const char *GetClassName() {return "vtkCompositeVolumeRayCaster";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set/Get the opacity transfer function that maps scalar value to
  // opacity.
  vtkSetObjectMacro(OpacityTransferFunction,vtkPiecewiseFunction);
  vtkGetObjectMacro(OpacityTransferFunction,vtkPiecewiseFunction);

  // Description:
  // Set/Get the value of Shading.
  vtkSetMacro( Shading, int );
  vtkGetMacro( Shading, int );
  vtkBooleanMacro( Shading, int );

  // Description:
  // Set/Get the ambient lighting coefficient.
  vtkSetClampMacro(Ambient,float,0.0,1.0);
  vtkGetMacro(Ambient,float);

  // Description:
  // Set/Get the diffuse lighting coefficient.
  vtkSetClampMacro(Diffuse,float,0.0,1.0);
  vtkGetMacro(Diffuse,float);

  // Description:
  // Set/Get the specular lighting coefficient.
  vtkSetClampMacro(Specular,float,0.0,1.0);
  vtkGetMacro(Specular,float);

  // Description:
  // Set/Get the specular power.
  vtkSetClampMacro(SpecularPower,float,0.0,100.0);
  vtkGetMacro(SpecularPower,float);

  // Description:
  // Set/Get the color type.  GetColorTypeAsString() returns one
  // of the following strings: "TransferFunction" or "Single".
  vtkSetClampMacro(ColorType,int,VTK_SINGLE_COLOR,VTK_TRANSFER_FUNCTION);
  vtkGetMacro(ColorType,int);
  void SetColorTypeToSingleColor(void) 
    {this->SetColorType(VTK_SINGLE_COLOR);};
  void SetColorTypeToTransferFunction(void)
    {this->SetColorType(VTK_TRANSFER_FUNCTION);};
  char *GetColorTypeAsString(void);

  // Description:
  // Set/Get the color to use when ColorType is SingleColor.
  vtkSetVector3Macro( SingleColor, float );
  vtkGetVectorMacro( SingleColor, float, 3 );

  vtkSetObjectMacro( ColorTransferFunction, vtkColorTransferFunction );
  vtkGetObjectMacro( ColorTransferFunction, vtkColorTransferFunction );

  float GetZeroOpacityThreshold( void );

  // These variables should be protected but are being
  // made public to be accessible to the templated function.
  // We used to have the templated function as a friend, but
  // this does not work with all compilers

  // The color type 0 = single color, 1 = transfer function color
  int                         ColorType;

  // The color of the volume
  // Used if SetSingleColor is called (also the default)
  float                       SingleColor[3];

  // The color transfer function - maps scalar value to RGB values
  // Used only if SetTransferFunctionColor is called
  vtkColorTransferFunction    *ColorTransferFunction;

  // The color transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a samples at each scalar value of the color transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                       *ColorTFArray;
  vtkTimeStamp                ColorTFArrayMTime;

  // The opacity transfer function - maps scalar value to opacity
  vtkPiecewiseFunction         *OpacityTransferFunction;

  // The opacity transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a samples at each scalar value of the opacity transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                       *OpacityTFArray;
  vtkTimeStamp                OpacityTFArrayMTime;

  // Number of elements in the OpacityTFArray
  int                         OpacityTFArraySize;

  // The corrected opacity transfer function array - this is identical
  // to the opacity transfer function array when the step size is 1.
  // In other cases, it is corrected to reflect the new material thickness
  // modelled by a step size different than 1.
  float                       *CorrectedOpacityTFArray;

  // CorrectedStepSize is the step size corrently modelled by
  // CorrectedTFArray.  It is used to determine when the 
  // CorrectedTFArray needs to be updated to match SampleDistance
  // in the volume mapper.
  float                       CorrectedStepSize;

  // CorrectedOTFArrayMTime - compared with OpacityTFArrayMTime for update
  vtkTimeStamp                CorrectedOTFArrayMTime;

  // The ambient, diffuse, and specular coeffients, and the specular power.
  // These are used only if shading is turned on
  float Ambient;
  float Diffuse;
  float Specular;
  float SpecularPower;

  // The normal encoder for creating/storing gradients and 
  // gradient magnitudes
  vtkNormalEncoder            NormalEncoder;

  // Shading indicator
  int                         Shading;

protected:

  // Description:
  // Give a ray type (0 = unsigned char, 1 = unsigned short,
  // 2 = short) cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final compositing value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha
  // pixel_value[4] = depth, and pixel_value[5] = number of steps
  void CastARay( int ray_type, void *data_ptr,
		 float ray_position[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] );

  // Description:
  // This is called from the Render method in vtkDepthPARCMapper, and
  // gives the vtkCompositeVolumeRayCaster a chance to do any specific
  // updating that it must do.  For example, it is here where we check
  // if the normals must be recomputed.
  void CasterUpdate( vtkRenderer *ren, vtkVolume *vol );

  // Description:
  // This method computes the corrected alpha blending for a given
  // step size.  The OpacityTFArray reflects step size 1.
  // The CorrectedOpacityTFArray reflects step size CorrectedStepSize.

  void UpdateOpacityTFforSampleSize(vtkRenderer *ren, vtkVolume *vol);

};

// Description:
// Return the correct color type string based on the
// ColorType instance variable value.
inline char *vtkCompositeVolumeRayCaster::GetColorTypeAsString(void)
{
  if ( this->ColorType == VTK_SINGLE_COLOR ) 
    {
    return "SingleColor";
    }
  else 
    {
    return "TransferFunction";
    }
}

#endif
