/*=========================================================================*/
// .NAME vtkVolumeProperty - represents the common properties for rendering a volume.
//
// .SECTION Description
// vtkVolumeProperty is used to represent common properties associated 
// with volume rendering. This includes properties for determining the type
// of interpolation to use when sampling a volume, the color of a volume, 
// the opacity of a volume, and shading parameters of a volume.

// .SECTION see also
// vtkPiecewiseFunction vtkColorTransferFunction


#ifndef __vtkVolumeProperty_h
#define __vtkVolumeProperty_h

#include "vtkObject.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkTimeStamp.h"

// Constants for InterpolationType
#define	VTK_NEAREST_INTERPOLATION	0
#define	VTK_LINEAR_INTERPOLATION	1

/****
// Constants for ShadeType
#define	VTK_NO_SHADE			0
#define	VTK_FAST_SHADE			1 // Fastest shading algorithm avail.
#define	VTK_BEST_SHADE			2 // Best shading algorithm available
*****/

class VTK_EXPORT vtkVolumeProperty : public vtkObject
{
public:
  vtkVolumeProperty();
  ~vtkVolumeProperty();
  static vtkVolumeProperty *New() {return new vtkVolumeProperty;};
  const char *GetClassName() {return "vtkVolumeProperty";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the interpolation type for sampling a volume.
  vtkSetClampMacro( InterpolationType, int,
	VTK_NEAREST_INTERPOLATION, VTK_LINEAR_INTERPOLATION);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToNearest() 
	{this->SetInterpolationType(VTK_NEAREST_INTERPOLATION);};
  void SetInterpolationTypeToLinear() 
	{this->SetInterpolationType(VTK_LINEAR_INTERPOLATION);};
  char *GetInterpolationTypeAsString(void);

  // Description:
  // Set the color of a volume to a gray level transfer function. This 
  // will also set the ColorChannels to 1.
  void SetColor( vtkPiecewiseFunction *function );

  // Description:
  // Get the number of color channels in the transfer function
  vtkGetMacro(ColorChannels,int);

  // Description:
  // Get the gray transfer function.
  vtkGetObjectMacro(GrayTransferFunction, vtkPiecewiseFunction);

  // Description:
  // Get the time that the GrayTransferFunction was set
  vtkGetMacro(GrayTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // Set the color of a volume to an RGB transfer function. This 
  // will also set the ColorChannels to 3.
  void SetColor( vtkColorTransferFunction *function );

  // Description:
  // Get the RGB transfer function.
  vtkGetObjectMacro(RGBTransferFunction, vtkColorTransferFunction);

  // Description:
  // Get the time that the RGBTransferFunction was set
  vtkGetMacro(RGBTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // Set the opacity of a volume to an opacity transfer function. 
  void SetOpacity( vtkPiecewiseFunction *function );

  // Description:
  // Get the opacity transfer function.
  vtkGetObjectMacro(OpacityTransferFunction, vtkPiecewiseFunction);

  // Description:
  // Get the time that the OpacityTransferFunction was set
  vtkGetMacro(OpacityTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // Set/Get the shading of a volume.
  vtkSetMacro(Shade,int);
  vtkGetMacro(Shade,int);
  vtkBooleanMacro(Shade,int);

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

protected:

  int				InterpolationType;

  int				ColorChannels;

  vtkPiecewiseFunction		*GrayTransferFunction;
  vtkTimeStamp			GrayTransferFunctionMTime;

  vtkColorTransferFunction	*RGBTransferFunction;
  vtkTimeStamp			RGBTransferFunctionMTime;

  vtkPiecewiseFunction		*OpacityTransferFunction;
  vtkTimeStamp			OpacityTransferFunctionMTime;

  int				Shade;
  float				Ambient;
  float 			Diffuse;
  float				Specular;
  float				SpecularPower;
};

// Description:
// Return the interpolation type as a descriptive character string.
inline char *vtkVolumeProperty::GetInterpolationTypeAsString(void)
{
  if( this->InterpolationType == VTK_NEAREST_INTERPOLATION )
    {
    return "Nearest Neighbor";
    }
  else if( this->InterpolationType == VTK_LINEAR_INTERPOLATION )
    {
    return "Linear";
    }
  else
    return "Unknown";
}

#endif
