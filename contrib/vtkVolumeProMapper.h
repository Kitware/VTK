/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProMapper.h
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
// .NAME vtkVolumeProMapper - Superclass for VolumePRO volume rendering mappers
//
// .SECTION Description
// vtkVolumeProMapper is the superclass for VolumePRO volume rendering mappers.
// Any functionality that is general across all VolumePRO implementations is
// placed here in this class. Subclasses of this class are for the specific
// board implementations. Subclasses of that are for underlying graphics 
// languages. Users should not create subclasses directly - 
// a vtkVolumeProMapper will automatically create the object of the right 
// type.
//
// If you do not have the VolumePRO libraries when building this object, then
// the New method will create a default renderer that will not render.
// You can check the NumberOfBoards ivar to see if it is a real rendering class.
// To build with the VolumePRO board see vtkVolumeProVG500Mapper.h for instructions.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.3dvolumegraphics.com/3dvolumegraphics/product/index.htm
//
// If you encouter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProVG500Mapper vtkOpenGLVolumeProVG500Mapper
//


#ifndef __vtkVolumeProMapper_h
#define __vtkVolumeProMapper_h

#include "vtkVolumeMapper.h"

//BTX
class VLIContext;
class VLIVolume;
class VLILookupTable;
class VLILight;
class VLICutPlane;
//ETX

#define VTK_BLEND_MODE_COMPOSITE        0
#define VTK_BLEND_MODE_MAX_INTENSITY    1
#define VTK_BLEND_MODE_MIN_INTENSITY    2

#define VTK_CURSOR_TYPE_CROSSHAIR       0
#define VTK_CURSOR_TYPE_PLANE           1

#define VTK_VOLUME_8BIT                 0
#define VTK_VOLUME_12BIT_UPPER          1
#define VTK_VOLUME_12BIT_LOWER          2

class VTK_EXPORT vtkVolumeProMapper : public vtkVolumeMapper
{
public:
  const char *GetClassName() {return "vtkVolumeProMapper";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Automatically create the proper subclass
  static vtkVolumeProMapper *New();

  // Description:
  // Render the image using the hardware and place it in the frame buffer
  virtual void Render( vtkRenderer *, vtkVolume * ) {}
  
  // Description:
  // The Renderer and RayCaster rely on the information to compose
  // images from various volume renderers
  int GetMapperType() { return VTK_FRAMEBUFFER_VOLUME_MAPPER; };

  // Description:
  // Set the blend mode
  vtkSetClampMacro( BlendMode, int,
	VTK_BLEND_MODE_COMPOSITE, VTK_BLEND_MODE_MIN_INTENSITY );
  vtkGetMacro(BlendMode,int);
  void SetBlendModeToComposite() 
	{this->SetBlendMode(VTK_BLEND_MODE_COMPOSITE);};
  void SetBlendModeToMaximumIntensity() 
	{this->SetBlendMode(VTK_BLEND_MODE_MAX_INTENSITY);};
  void SetBlendModeToMinimumIntensity() 
	{this->SetBlendMode(VTK_BLEND_MODE_MIN_INTENSITY);};
  const char *GetBlendModeAsString(void);

  // Description:
  // Set the subvolume
  vtkSetVector6Macro( SubVolume, int );
  vtkGetVectorMacro( SubVolume, int, 6 );

  // Description:
  // Turn the cursor on / off
  vtkSetClampMacro( Cursor, int, 0, 1 );
  vtkGetMacro( Cursor, int );
  vtkBooleanMacro( Cursor, int );

  // Description:
  // Set the type of the cursor
  vtkSetClampMacro( CursorType, int, 
		    VTK_CURSOR_TYPE_CROSSHAIR, VTK_CURSOR_TYPE_PLANE );
  vtkGetMacro( CursorType, int );
  void SetCursorTypeToCrossHair()
    { this->SetCursorType( VTK_CURSOR_TYPE_CROSSHAIR ); };
  void SetCursorTypeToPlane()
    { this->SetCursorType( VTK_CURSOR_TYPE_PLANE ); };
  const char *GetCursorTypeAsString( void );

  // Description:
  // Set/Get the cursor position
  vtkSetVector3Macro( CursorPosition, double );
  vtkGetVectorMacro( CursorPosition, double, 3 );

  // Description:
  // Turn supersampling on/off
  vtkSetClampMacro( SuperSampling, int, 0, 1 );
  vtkGetMacro( SuperSampling, int );
  vtkBooleanMacro( SuperSampling, int );

  // Description:
  // Set the supersampling factors
  void SetSuperSamplingFactor( double x, double y, double z );
  void SetSuperSamplingFactor( double f[3] )
    { this->SetSuperSamplingFactor( f[0], f[1], f[2] ); };
  vtkGetVectorMacro( SuperSamplingFactor, double, 3 );

  // Description:
  // Turn on / off the cut plane
  vtkSetClampMacro( CutPlane, int, 0, 1 );
  vtkGetMacro( CutPlane, int );
  vtkBooleanMacro( CutPlane, int );

  // Description:
  // Set/Get the cut plane equation
  vtkSetVector4Macro( CutPlaneEquation, double );
  vtkGetVectorMacro( CutPlaneEquation, double, 4 );
  
  // Description:
  // Set / Get the cut plane thickness
  vtkSetClampMacro( CutPlaneThickness, double, 0.0, 9.99e10 );
  vtkGetMacro( CutPlaneThickness, double );

  // Description:
  // Set / Get the cut plane falloff value for intensities
  vtkSetClampMacro( CutPlaneFallOffDistance, int, 0.0, 16 );
  vtkGetMacro( CutPlaneFallOffDistance, int );

  // Description:
  // Set/Get the gradient magnitude opacity modulation 
  vtkSetClampMacro( GradientOpacityModulation, int, 0, 1 );
  vtkGetMacro( GradientOpacityModulation, int );
  vtkBooleanMacro( GradientOpacityModulation, int );

  // Description:
  // Set/Get the gradient magnitude diffuse modulation 
  vtkSetClampMacro( GradientDiffuseModulation, int, 0, 1 );
  vtkGetMacro( GradientDiffuseModulation, int );
  vtkBooleanMacro( GradientDiffuseModulation, int );

  // Description:
  // Set/Get the gradient magnitude specular modulation 
  vtkSetClampMacro( GradientSpecularModulation, int, 0, 1 );
  vtkGetMacro( GradientSpecularModulation, int );
  vtkBooleanMacro( GradientSpecularModulation, int );

  // Description:
  // Conveniece methods for debugging
  vtkGetMacro( NoHardware, int );
  vtkGetMacro( WrongVLIVersion, int );
  
  // Description:
  // Access methods for some board info
  vtkGetMacro( NumberOfBoards, int );
  vtkGetMacro( MajorBoardVersion, int );
  vtkGetMacro( MinorBoardVersion, int );
  virtual int GetAvailableBoardMemory() { return 0; }
  virtual void GetLockSizesForBoardMemory( unsigned int type,
					   unsigned int *xSize,
					   unsigned int *ySize,
					   unsigned int *zSize ) {};
 
protected:
  vtkVolumeProMapper();
  ~vtkVolumeProMapper();
  vtkVolumeProMapper(const vtkVolumeProMapper&) {};
  void operator=(const vtkVolumeProMapper&) {};
 // Make sure everything is OK for rendering
  int StatusOK();

  // The volume context - create it once and keep it around
  VLIContext           *Context;

  // The Volume, and the Input that was used to build the volume
  // and the time at which it was last built.
  VLIVolume            *Volume;
  vtkStructuredPoints  *VolumeInput;
  vtkTimeStamp         *VolumeBuildTime;

  // The type of data in the volume - 8bit, 12bit upper, or 12bit lower
  int                  VolumeDataType;

  // The lookup table for RGBA - create it once then modify it as 
  // necessary
  VLILookupTable       *LookupTable;

  // The blending mode to use
  int                  BlendMode;

  // The lights, and how many of them there are. Not all of them
  // are turned on or used.
  VLILight             **Lights;
  int                  NumberOfLights;

  // The subvolume extent (xmin, xmax, ymin, ymax, zmin, zmax)
  int                  SubVolume[6];

  // The cursor parameters
  int                  Cursor;
  int                  CursorType;
  double               CursorPosition[3];

  // The cut plane parameters
  int                  CutPlane;
  VLICutPlane          *Cut;
  double               CutPlaneEquation[4];
  double               CutPlaneThickness;
  int                  CutPlaneFallOffDistance;

  // The supersampling parameters
  int                  SuperSampling;
  double               SuperSamplingFactor[3];

  // The gradient modulation flags
  int                  GradientOpacityModulation;
  int                  GradientDiffuseModulation;
  int                  GradientSpecularModulation;

  // Some board properties
  int                  NumberOfBoards;
  int                  MajorBoardVersion;
  int                  MinorBoardVersion;
  int                  GradientTableSize;

  // Some error conditions that may occur during initialization
  int                  NoHardware;
  int                  WrongVLIVersion;
  int                  DisplayedMessage;

};

// Description:
// Get the blending mode as a descriptive string
inline const char *vtkVolumeProMapper::GetBlendModeAsString()
{
  switch ( this->BlendMode )
    {
    case VTK_BLEND_MODE_COMPOSITE:
      return "Composite";
    case VTK_BLEND_MODE_MAX_INTENSITY:
      return "Maximum Intensity";
    case VTK_BLEND_MODE_MIN_INTENSITY:
      return "Minimum Intensity";
    default:
      return "Unknown Blend Mode";
    }
}

// Description:
// Get the cursor type as a descriptive string
inline const char *vtkVolumeProMapper::GetCursorTypeAsString()
{
  switch ( this->CursorType )
    {
    case VTK_CURSOR_TYPE_CROSSHAIR:
      return "Crosshair";
    case VTK_CURSOR_TYPE_PLANE:
      return "Plane";
    default:
      return "Unknown Cursor Type";
    }
}

#endif

