/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIsoVolumeRayCaster.h
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
// .NAME vtkIsoVolumeRayCaster - Creates display of isosurface
// .SECTION Description
// vtkIsoVolumeRayCaster is a concrete implementation of vtkVolumeRayCaster that
// creates display of isosurface of scalar data. The appearance of
// the Iso is controlled by LinearRampRange and LinearRampValues

// .SECTION see also
// vtkDepthPARCMapper

#ifndef __vtkIsoVolumeRayCaster_h
#define __vtkIsoVolumeRayCaster_h

#include "vtkVolumeRayCaster.h"
#include "vtkVolume.h"
#include "vtkColorTransferFunction.h"
#include "vtkNormalEncoder.h"
#include "vtkPolyData.h"

#define VTK_SINGLE_COLOR 0
#define VTK_TRANSFER_FUNCTION 1

class VTK_EXPORT vtkIsoVolumeRayCaster : public vtkVolumeRayCaster
{
public:
  vtkIsoVolumeRayCaster();
  ~vtkIsoVolumeRayCaster();
  static vtkIsoVolumeRayCaster *New() {return new vtkIsoVolumeRayCaster;};
  const char *GetClassName() {return "vtkIsoVolumeRayCaster";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set/Get the value of IsoValue.
  vtkSetMacro( IsoValue, float );
  vtkGetMacro( IsoValue, float );

  // Description:
  // Set/Get the opcaity transfer function that maps scalar value to
  // opacity
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
  vtkSetClampMacro(ColorType,int,VTK_TRANSFER_FUNCTION,VTK_SINGLE_COLOR);
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

  // Description:
  // The value of isosurface in volume
  float                       IsoValue;
  // Description:
  // The color of isosurface in volume
  float                       Color[3];

protected:

  // Description:
  // Give a ray type (0 = unsigned char, 1 = unsigned short,
  // 2 = short) cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha
  // pixel_value[4] = depth, and pixel_value[5] = number of steps
  void CastARay( int ray_type, void *data_ptr,
		 float ray_position[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] );

  // Description:
  // This is called from the Render method in vtkDepthPARCMapper, and
  // gives the vtkIsoVolumeRayCaster a chance to do any specific
  // updating that it must do.  For example, it is here where we check
  // if the normals must be recomputed.
  void CasterUpdate( vtkRenderer *ren, vtkVolume *vol );

};

// Description:
// Return the correct color type string based on the
// ColorType instance variable value.
inline char *vtkIsoVolumeRayCaster::GetColorTypeAsString(void)
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
