/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineIntegralConvolution2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLineIntegralConvolution2D - GPU-based implementation of Line 
//  Integral Convolution (LIC)
//
// .SECTION Description
//  This class resorts to GLSL to implement GPU-based Line Integral Convolution 
//  (LIC) for visualizing a 2D vector field that may be obtained by projecting
//  an original 3D vector field onto a surface (such that the resulting 2D 
//  vector at each grid point on the surface is tangential to the local normal,
//  as done in vtkSurfaceLICPainter). 
//  
//  As an image-based technique, 2D LIC works by (1) integrating a bidirectional
//  streamline from the center of each pixel (of the LIC output image), (2)
//  locating the pixels along / hit by this streamline as the correlated pixels
//  of the starting pixel (seed point / pixel), (3) indexing a (usually white) 
//  noise texture (another input to LIC, in addition to the 2D vector field, 
//  usually with the same size as that of the 2D vetor field) to determine the
//  values (colors) of these pixels (the starting and the correlated pixels), 
//  typically through bi-linear interpolation, and (4) performing convolution
//  (weighted averaging) on these values, by adopting a low-pass filter (such 
//  as box, ramp, and Hanning kernels), to obtain the result value (color) that
//  is then assigned to the seed pixel.
//
//  The GLSL-based GPU implementation herein maps the aforementioned pipeline to
//  fragment shaders and a box kernel is employed. Both the white noise and the
//  vector field are provided to the GPU as texture objects (supported by the 
//  multi-texturing capability). In addition, there are four texture objects 
//  (color buffers) allocated to constitute two pairs that work in a ping-pong
//  fashion, with one as the read buffers and the other as the write / render 
//  targets. Maintained by a frame buffer object (GL_EXT_framebuffer_object),
//  each pair employs one buffer to store the current (dynamically updated)
//  position (by means of the texture coordinate that keeps being warped by the
//  underlying vector) of the (virtual) particle initially released from each 
//  fragment while using the bother buffer to store the current (dynamically
//  updated too) accumulated texture value that each seed fragment (before the
//  'mesh' is warped) collects. Given NumberOfSteps integration steps in each
//  direction, there are a total of (2 * NumberOfSteps + 1) fragments (including
//  the seed fragment) are convolved and each contributes 1 / (2 * NumberOfSteps
//  + 1) of the associated texture value to fulfill the box filter.
//  
//  One pass of LIC (basic LIC) tends to produce low-contrast / blurred images and
//  vtkLineIntegralConvolution2D provides an option for creating enhanced LIC
//  images. Enhanced LIC improves image quality by increasing inter-streamline
//  contrast while suppressing artifacts. It performs two passes of LIC, with a
//  3x3 Laplacian high-pass filter in between that processes the output of pass
//  #1 LIC and forwards the result as the input 'noise' to pass #2 LIC. Enhanced
//  LIC automatically degenerates to basic LIC during user interaction.
//
//  vtkLineIntegralConvolution2D applies masking to zero-vector fragments so
//  that un-filtered white noise areas are made totally transparent by class
//  vtkSurfaceLICPainter to show the underlying geometry surface.
//
// .SECTION Required OpenGL Extensins
//  GL_ARB_texture_non_power_of_two
//  GL_VERSION_2_0
//  GL_ARB_texture_float
//  GL_ARB_draw_buffers
//  GL_EXT_framebuffer_object
//
// .SECTION See Also
//  vtkSurfaceLICPainter vtkImageDataLIC2D vtkStructuredGridLIC2D

#ifndef __vtkLineIntegralConvolution2D_h
#define __vtkLineIntegralConvolution2D_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkTextureObject;

class VTK_RENDERING_EXPORT vtkLineIntegralConvolution2D : public vtkObject
{
public:

  static vtkLineIntegralConvolution2D * New();
  vtkTypeMacro( vtkLineIntegralConvolution2D, vtkObject );
  void PrintSelf( ostream & os, vtkIndent indent );
  
  // Description:
  // Enable/Disable enhanced LIC that improves image quality by increasing
  // inter-streamline contrast while suppressing artifacts. Enhanced LIC
  // performs two passes of LIC, with a 3x3 Laplacian high-pass filter in
  // between that processes the output of pass #1 LIC and forwards the result
  // as the input 'noise' to pass #2 LIC. This flag is automatically turned
  // off during user interaction.
  vtkSetMacro( EnhancedLIC, int );
  vtkGetMacro( EnhancedLIC, int );
  vtkBooleanMacro( EnhancedLIC, int );
  
  //Description:
  // Enable/Disable LICForSurface, for which the LIC texture is composited
  // with the underlying geometry.
  vtkSetMacro( LICForSurface, int );
  vtkGetMacro( LICForSurface, int );
  vtkBooleanMacro( LICForSurface, int );
  
  // Description:
  // Number of streamline integration steps (initial value is 1).
  // In term of visual quality, the greater (within some range) the better.
  vtkSetMacro( NumberOfSteps, int );
  vtkGetMacro( NumberOfSteps, int );
  
  // Description:
  // Get/Set the streamline integration step size (0.01 by default). This is 
  // the length of each step in normalized image space i.e. in range [0, 1].
  // In term of visual quality, the smaller the better. The type for the 
  // interface is double as VTK interface is, but GPU only supports float.
  // Thus it will be converted to float in the execution of the algorithm.
  vtkSetClampMacro( LICStepSize, double, 0.0, 1.0 );
  vtkGetMacro( LICStepSize, double );
  
  // Description:
  // Set/Get the input white noise texture (initial value is NULL).
  void SetNoise( vtkTextureObject * noise );
  vtkGetObjectMacro( Noise, vtkTextureObject );
  
  // Description:
  // Set/Get the vector field (initial value is NULL).
  void SetVectorField( vtkTextureObject * vectorField );
  vtkGetObjectMacro( VectorField, vtkTextureObject );
  
  // Description:
  // If VectorField has >= 3 components, we must choose which 2 components 
  // form the (X, Y) components for the vector field. Must be in the range 
  // [0, 3].
  vtkSetVector2Macro( ComponentIds, int );
  vtkGetVector2Macro( ComponentIds, int );

  // Description:
  // Set/Get the spacing in each dimension of the plane on which the vector 
  // field is defined. This class performs LIC in the normalized image space
  // and hence generally it needs to transform the input vector field (given 
  // in physical space) to the normalized image space. The Spacing is needed
  // to determine the tranform. Default is (1.0, 1.0). It is possible to 
  // disable vector transformation by setting TransformVectors to 0.
  vtkSetVector2Macro( GridSpacings, double );
  vtkGetVector2Macro( GridSpacings, double );
  
  // Description:
  // This class performs LIC in the normalized image space. Hence, by default
  // it transforms the input vectors to the normalized image space (using the
  // GridSpacings and input vector field dimensions). Set this to 0 to disable
  // tranformation if the vectors are already tranformed.
  vtkSetClampMacro( TransformVectors, int, 0, 1 );
  vtkBooleanMacro( TransformVectors, int );
  vtkGetMacro( TransformVectors, int );
  
  // Description:
  // The the magnification factor (default is 1.0).
  vtkSetClampMacro( Magnification, int, 1, VTK_INT_MAX );
  vtkGetMacro( Magnification, int );
  
  // Description:
  // On machines where the vector field texture is clamped between [0,1], one 
  // can specify the shift/scale factor used to convert the original vector 
  // field to lie in the clamped range. Default is (0.0, 1.0);
  void SetVectorShiftScale( double shift, double scale )
    {
    this->VectorShift = shift;
    this->VectorScale = scale;
    this->Modified();
    }
    
  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported( vtkRenderWindow * renWin );
  
  // Description:
  // Perform the LIC and obtain the LIC texture. Return 1 if no error.
  int Execute();

  // Description:
  // Same as Execute() except that the LIC operation is performed only on a
  // window (given by the \c extent) in the input VectorField. The \c extent
  // is relative to the input VectorField. The output LIC image will be of 
  // the size specified by extent.
  int Execute( unsigned int extent[4] );
  int Execute( int extent[4] );
  
  // Description:
  // LIC texture (initial value is NULL) set by Execute().
  void SetLIC( vtkTextureObject * lic );
  vtkGetObjectMacro( LIC, vtkTextureObject );
 
protected:
   vtkLineIntegralConvolution2D();
  ~vtkLineIntegralConvolution2D();
 
  int     Magnification;
  int     NumberOfSteps;
  int     LICForSurface;
  int     EnhancedLIC;
  double  LICStepSize;
  double  VectorShift;
  double  VectorScale;
  
  int     TransformVectors;
  int     ComponentIds[2];
  double  GridSpacings[2];
  
  vtkTextureObject * VectorField;
  vtkTextureObject * Noise;
  vtkTextureObject * LIC;
  
private:
  vtkLineIntegralConvolution2D( const vtkLineIntegralConvolution2D & ); // Not implemented.
  void operator = ( const vtkLineIntegralConvolution2D & );             // Not implemented.
//ETX
};

#endif
