/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "vtkProp.h"
#include "vtkImageData.h"

class vtkPropCollection;
class vtkRenderer;

class VTK_EXPORT vtkImageActor : public vtkProp
{
public:
  vtkTypeMacro(vtkImageActor,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the image actor.
  static vtkImageActor *New();

  // Description:
  // Set/Get the image data input for the image actor.  
  vtkSetObjectMacro(Input,vtkImageData);
  vtkGetObjectMacro(Input,vtkImageData);

  // Description:
  // Turn on/off linear interpolation of the image when rendering.
  vtkGetMacro(Interpolate,int);
  vtkSetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

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
  float *GetBounds();
  void GetBounds(float bounds[6]);

  // Description:
  // Return a slice number computed from the display extent.
  int GetSliceNumber();
  
//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods.
  // int RenderTranslucentGeometry(vtkViewport *viewport);
  int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual void Load(vtkRenderer *) {};
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
  vtkImageActor(const vtkImageActor&);
  void operator=(const vtkImageActor&);

  int           Interpolate;
  vtkImageData* Input;
  int           DisplayExtent[6];
  float         Bounds[6];
};

#endif

