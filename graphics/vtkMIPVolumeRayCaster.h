/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMIPVolumeRayCaster.h
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
// .NAME vtkMIPVolumeRayCaster - Creates maximum intensity projections using Depth Parc acceleration
// .SECTION Description
// vtkMIPVolumeRayCaster is a concrete implementation of vtkDepthPARCMapper that
// creates maximum intensity projections of scalar data. The appearance of
// the MIP is controlled by LinearRampRange and LinearRampValues

// .SECTION see also
// vtkDepthPARCMapper

#ifndef __vtkMIPVolumeRayCaster_h
#define __vtkMIPVolumeRayCaster_h

#include "vtkVolumeRayCaster.h"
#include "vtkVolume.h"
#include "vtkColorTransferFunction.h"
#include "vtkPolyData.h"

#define VTK_SINGLE_COLOR 0
#define VTK_TRANSFER_FUNCTION 1

class VTK_EXPORT vtkMIPVolumeRayCaster : public vtkVolumeRayCaster
{
public:
  vtkMIPVolumeRayCaster();
  ~vtkMIPVolumeRayCaster();
  static vtkMIPVolumeRayCaster *New() {return new vtkMIPVolumeRayCaster;};
  const char *GetClassName() {return "vtkMIPVolumeRayCaster";};
  void PrintSelf( ostream& os, vtkIndent index );

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
  // Set/Get the range of the linear ramp that maps scalar value to
  // intensity value.  All scalar values below LinearRampRange[0] have
  // an intensity of 0.  Values between LinearRampRange[0] and
  // LinearRampRange[1] have intensity values that vary linearly from
  // LinearRampValue[0] to LinearRampValue[1]. Scalar values above
  // LinearRampRange[1] have an intensity value of LinearRampValue[1].
  vtkSetVector2Macro( LinearRampRange, float ); 
  vtkGetVectorMacro( LinearRampRange, float, 2 ); 

  // Description:
  // Set/Get the endpoint values of the linear ramp that maps scalar
  // value to intensity value. See above for complete description.
  vtkSetVector2Macro( LinearRampValue, float ); 
  vtkGetVectorMacro( LinearRampValue, float, 2 ); 

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
  int                        ColorType;

  // The color of the volume
  // Used if SetSingleColor is called (also the default)
  float                      SingleColor[3];

  float                      LinearRampRange[2];
  float                      LinearRampValue[2];

  // The color transfer function - maps scalar value to RGB values
  // Used only if SetTransferFunctionColor is called
  vtkColorTransferFunction   *ColorTransferFunction;

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
  // Build the polygonal data for the PARC approximation
  vtkPolyData *BuildPARCPolyData( void );

  // Description:
  // This is called from the Render method in vtkDepthPARCMapper, and
  // gives the vtkCompositeDPARCMapper a chance to do any specific
  // updating that it must do.  For example, it is here where we check
  // if the normals must be recomputed.
  void CasterUpdate( vtkRenderer *ren, vtkVolume *vol );

};

// Description:
// Return the correct color type string based on the
// ColorType variable value.
inline char *vtkMIPVolumeRayCaster::GetColorTypeAsString(void)
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
