/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageActor - draw an image (data & properties) in a rendered 3D scene
// .SECTION Description
// vtkImageActor is used to render an image in a 3D scene.  The image
// is placed at the origin of the image, and its size is controlled by the
// image dimensions and image spacing. The orientation of the image is
// orthogonal to one of the x-y-z axes depending on which plane the
// image is defined in. vtkImageActor duplicates the functionality 
// of combinations of other VTK classes in a convenient, single class.

// .SECTION Caveats
// vtkImageData requires the image to be of type unsigned char. Use a
// filter like vtkImageShiftScale to convert to unsigned char (the
// method to use is SetOutputTypeToUnsignedChar()).

// .SECTION See Also
// vtkImageData vtkProp vtkImageShiftScale

#ifndef __vtkImageActor_h
#define __vtkImageActor_h

#include "vtkProp3D.h"

class vtkPropCollection;
class vtkRenderer;
class vtkImageData;

class VTK_RENDERING_EXPORT vtkImageActor : public vtkProp3D
{
public:
  vtkTypeMacro(vtkImageActor,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the image actor.
  static vtkImageActor *New();

  // Description:
  // Set/Get the image data input for the image actor.  
  virtual void SetInput(vtkImageData *);
  vtkGetObjectMacro(Input,vtkImageData);

  // Description:
  // Turn on/off linear interpolation of the image when rendering.
  vtkGetMacro(Interpolate,int);
  vtkSetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

  // Description:
  // Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
  // transparent.
  vtkSetClampMacro(Opacity,double,0.0,1.0);
  vtkGetMacro(Opacity,double);

  // Description:
  // The image extent is generally set explicitly, but if not set
  // it will be determined from the input image data.
  void SetDisplayExtent(int extent[6]);
  void SetDisplayExtent(int minX, int maxX, int minY, int maxY, 
                        int minZ, int maxZ);
  void GetDisplayExtent(int extent[6]);
  int *GetDisplayExtent() {return this->DisplayExtent;}

  // Description:
  // Get the bounds of this image actor. Either copy the bounds
  // into a user provided array or return a pointer to an array.
  // In either case the boudns is expressed as a 6-vector 
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->Superclass::GetBounds(bounds); };

  // Description:
  // Get the bounds of the data that is displayed by this image
  // actor.  If the transformation matrix for this actor is the
  // identity matrix, this will return the same value as
  // GetBounds.
  double *GetDisplayBounds();
  void GetDisplayBounds(double bounds[6]);

  // Description:
  // Return the slice number (& min/max slice number) computed from the display
  // extent.
  int GetSliceNumber();
  int GetSliceNumberMax();
  int GetSliceNumberMin();
  
  //BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods.
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual void Render(vtkRenderer *) {};
  
  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();
  //ETX

  // Description:
  // Set/Get the current slice number. The axis Z in ZSlice does not
  // necessarily have any relation to the z axis of the data on disk.
  // It is simply the axis orthogonal to the x,y, display plane.
  // GetWholeZMax and Min are convenience methods for obtaining
  // the number of slices that can be displayed. Again the number
  // of slices is in reference to the display z axis, which is not
  // necessarily the z axis on disk. (due to reformatting etc)
  void SetZSlice(int z) {this->SetDisplayExtent(
    this->DisplayExtent[0], this->DisplayExtent[1],
    this->DisplayExtent[2], this->DisplayExtent[3], z, z);
  };
  int GetZSlice() { return this->DisplayExtent[4];};
  int GetWholeZMin();
  int GetWholeZMax();

protected:
  vtkImageActor();
  ~vtkImageActor();

  int           Interpolate;
  double        Opacity;
  vtkImageData* Input;
  int           DisplayExtent[6];
  int           ComputedDisplayExtent[6];
  double        DisplayBounds[6];

private:
  vtkImageActor(const vtkImageActor&);  // Not implemented.
  void operator=(const vtkImageActor&);  // Not implemented.
};

#endif

