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
  const char *GetClassName() {return "vtkVolumeMapper";};
  void PrintSelf( ostream& os, vtkIndent index );

  void operator=(const vtkVolumeMapper& mapper);

  virtual void Render(vtkRenderer *ren, vtkVolume *vol) = 0;

  virtual void Update();

  // Description:
  // Get the bounds of the scalar data.
  virtual float *GetBounds();

  // Description:
  // Will the hardware color and z buffers be destroyed during a render?
  virtual int DestroyHardwareBuffer( void ) = 0;

  // Description:
  // Will the image be in hardware when the render is complete?
  virtual int ImageLocatedInHardware( void ) = 0;

  // Description:
  // Get the z buffer data for the image.
  virtual float *GetZbufferData( void ) = 0;

  // Description:
  // Get the RGBA color buffer data for the image.
  virtual float *GetRGBAPixelData( void ) = 0;

  // Description:
  // Turn On/Off orthogonal clipping. (Clipping planes are
  // perpendicular to the coordinate axes.)
  vtkSetMacro(Clipping,int);
  vtkGetMacro(Clipping,int);
  vtkBooleanMacro(Clipping,int);

  float GetXminClipPlane( void ) { return this->ClippingPlanes[0]; };
  float GetYminClipPlane( void ) { return this->ClippingPlanes[1]; };
  float GetZminClipPlane( void ) { return this->ClippingPlanes[2]; };
  float GetXmaxClipPlane( void ) { return this->ClippingPlanes[3]; };
  float GetYmaxClipPlane( void ) { return this->ClippingPlanes[4]; };
  float GetZmaxClipPlane( void ) { return this->ClippingPlanes[5]; };

  // Description:
  // Set/Get the ClippingPlanes ( xmin, ymin, zmin, xmax, ymax, zmax )
  void SetClippingPlanes( float a, float b, float c, 
                          float d, float e, float f );
  void SetClippingPlanes( float p[6] ); 
  float *GetClippingPlanes( void ) { return this->ClippingPlanes; };

  // Description:
  // Set/Get the scalar input data
  vtkSetObjectMacro( ScalarInput, vtkStructuredPoints );
  void SetScalarInput(vtkImageCache *cache)
    {this->SetScalarInput(cache->GetImageToStructuredPoints()->GetOutput());}
  virtual vtkStructuredPoints *GetScalarInput() {return this->ScalarInput;};

protected:
  vtkStructuredPoints  *ScalarInput;
  int                  Clipping;
  float                ClippingPlanes[6];
  vtkTimeStamp         BuildTime;
};

inline void vtkVolumeMapper::SetClippingPlanes( 
                     float a, float b, float c, float d, float e, float f )
{
  this->ClippingPlanes[0] = a;
  this->ClippingPlanes[1] = b;
  this->ClippingPlanes[2] = c;
  this->ClippingPlanes[3] = d;
  this->ClippingPlanes[4] = e;
  this->ClippingPlanes[5] = f;
}

inline void vtkVolumeMapper::SetClippingPlanes( float p[6] )
{
  this->ClippingPlanes[0] = p[0];
  this->ClippingPlanes[1] = p[1];
  this->ClippingPlanes[2] = p[2];
  this->ClippingPlanes[3] = p[3];
  this->ClippingPlanes[4] = p[4];
  this->ClippingPlanes[5] = p[5];
}

#endif


