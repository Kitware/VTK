/*=========================================================================*/
// .NAME vtkVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeMapper is the abstract definition of a volume mapper.
// All volume mappers must answer DestroyHardwareBuffer which indicates
// whether or not the hardware color and z buffers will be destroyed
// during the volume's render method, and ImageLocatedInHardware which
// indicates if the image will be in the hardware color and z buffers or
// should be obtained through the GetZbufferData and GetRGBAPixelData
// methods. In addition, every mapper must supply the bounds of its
// data.

// .SECTION see also
// vtkDepthPARCMapper vtkMIPDPARCMapper

#ifndef __vtkVolumeMapper_h
#define __vtkVolumeMapper_h

#include "vtkObject.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkStructuredPoints.h"
#include "vtkImageToStructuredPoints.h"

class vtkRenderer;
class vtkVolume;

class VTK_EXPORT vtkVolumeMapper : public vtkObject
{
public:
  vtkVolumeMapper();
  ~vtkVolumeMapper();
  char *GetClassName() {return "vtkVolumeMapper";};
  void PrintSelf( ostream& os, vtkIndent index );

  void operator=(const vtkVolumeMapper& mapper);

  virtual void Render(vtkRenderer *ren, vtkVolume *vol) = 0;

  virtual void Update();

  // Description:
  // Get the bounds of the scalar data
  virtual float *GetBounds();

  // Description:
  // Will the hardware color and z buffers be destroyed during a render?
  virtual int DestroyHardwareBuffer( void ) = 0;

  // Description:
  // Will the image be in hardware when the render is complete?
  virtual int ImageLocatedInHardware( void ) = 0;

  // Description:
  // Get the z buffer data for the image
  virtual float *GetZbufferData( void ) = 0;

  // Description:
  // Get the RGBA color buffer data for the image
  virtual float *GetRGBAPixelData( void ) = 0;

  // Description:
  // Turn On/Off OrthoClipping
  vtkGetMacro( OrthoClippingEnabled, int );
  void EnableOrthoClipping( void )  { this->OrthoClippingEnabled = 1; };
  void DisableOrthoClipping( void ) { this->OrthoClippingEnabled = 0; };


  float GetXminOrthoClipPlane( void ) { return this->OrthoClippingPlanes[0]; };
  float GetXmaxOrthoClipPlane( void ) { return this->OrthoClippingPlanes[1]; };
  float GetYminOrthoClipPlane( void ) { return this->OrthoClippingPlanes[2]; };
  float GetYmaxOrthoClipPlane( void ) { return this->OrthoClippingPlanes[3]; };
  float GetZminOrthoClipPlane( void ) { return this->OrthoClippingPlanes[4]; };
  float GetZmaxOrthoClipPlane( void ) { return this->OrthoClippingPlanes[5]; };

  // Description:
  // Set/Get the OrthoClippingPlanes ( xmin, ymin, zmin, xmax, ymax, zmax )
  void SetOrthoClippingPlanes( float a, float b, float c, 
			       float d, float e, float f );
  void SetOrthoClippingPlanes( float p[6] ); 
  float *GetOrthoClippingPlanes( void ) { return this->OrthoClippingPlanes; };

  // Description:
  // Set/Get the scalar input data
  vtkSetObjectMacro( ScalarInput, vtkStructuredPoints );
  void SetScalarInput(vtkImageSource *cache)
    {this->SetScalarInput(cache->GetImageToStructuredPoints()->GetOutput());}
  virtual vtkStructuredPoints *GetScalarInput() {return this->ScalarInput;};

protected:
  vtkStructuredPoints  *ScalarInput;
  int                  OrthoClippingEnabled;
  float                OrthoClippingPlanes[6];

  vtkTimeStamp BuildTime;
};

inline void vtkVolumeMapper::SetOrthoClippingPlanes( 
                     float a, float b, float c, float d, float e, float f )
{
  this->OrthoClippingPlanes[0] = a;
  this->OrthoClippingPlanes[1] = b;
  this->OrthoClippingPlanes[2] = c;
  this->OrthoClippingPlanes[3] = d;
  this->OrthoClippingPlanes[4] = e;
  this->OrthoClippingPlanes[5] = f;
}

inline void vtkVolumeMapper::SetOrthoClippingPlanes( float p[6] )
{
  this->OrthoClippingPlanes[0] = p[0];
  this->OrthoClippingPlanes[1] = p[1];
  this->OrthoClippingPlanes[2] = p[2];
  this->OrthoClippingPlanes[3] = p[3];
  this->OrthoClippingPlanes[4] = p[4];
  this->OrthoClippingPlanes[5] = p[5];
}

#endif


