/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexture.h
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
// .NAME vtkTexture - handles properties associated with a texture map
// .SECTION Description
// vtkTexture is an object that handles loading and binding of texture
// maps. It obtains its data from an input image data dataset type.
// Thus you can create visualization pipelines to read, process, and 
// construct textures. Note that textures will only work if texture
// coordinates are also defined, and if the rendering system supports 
// texture.
//
// Instances of vtkTexture are associated with actors via the actor's
// SetTexture() method. Actors can share texture maps (this is encouraged
// to save memory resources.) 

// .SECTION Caveats
// Currently only 2D texture maps are supported, even though the data pipeline
// supports 1,2, and 3D texture coordinates. 
// 
// Some renderers such as OpenGL require that the texture map dimensions are
// a power of two in each direction. Other renderers may have similar
// (ridiculous) restrictions, so be careful out there... (Note: a recent change
// to vtk allows use of non-power of two texture maps in OpenGL. The texture is
// automatically resampled to a power of two in one or more directions.)

// .SECTION See Also
// vtkActor vtkRenderer vtkOpenGLTexture

#ifndef __vtkTexture_h
#define __vtkTexture_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"

class vtkRenderer;
class vtkWindow;
class vtkUnsignedCharArray;

#define VTK_TEXTURE_QUALITY_DEFAULT 0
#define VTK_TEXTURE_QUALITY_16BIT   16
#define VTK_TEXTURE_QUALITY_32BIT   32

class VTK_RENDERING_EXPORT vtkTexture : public vtkProcessObject
{
public:
  static vtkTexture *New();
  vtkTypeMacro(vtkTexture,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Renders a texture map. It first checks the object's modified time
  // to make sure the texture maps Input is valid, then it invokes the 
  // Load() method.
  virtual void Render(vtkRenderer *ren);

  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};

  // Description:
  // Abstract interface to renderer. Each concrete subclass of 
  // vtkTexture will load its data into graphics system in response 
  // to this method invocation.
  virtual void Load(vtkRenderer *) {};

  // Description:
  // Turn on/off the repetition of the texture map when the texture
  // coords extend beyond the [0,1] range.
  vtkGetMacro(Repeat,int);
  vtkSetMacro(Repeat,int);
  vtkBooleanMacro(Repeat,int);

  // Description:
  // Turn on/off linear interpolation of the texture map when rendering.
  vtkGetMacro(Interpolate,int);
  vtkSetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

  // Description:
  // Force texture quality to 16-bit or 32-bit.
  // This might not be supported on all machines.
  vtkSetMacro(Quality,int);
  vtkGetMacro(Quality,int);
  void SetQualityToDefault() {this->SetQuality(VTK_TEXTURE_QUALITY_DEFAULT);};
  void SetQualityTo16Bit() {this->SetQuality(VTK_TEXTURE_QUALITY_16BIT);};
  void SetQualityTo32Bit() {this->SetQuality(VTK_TEXTURE_QUALITY_32BIT);};

  // Description:
  // Turn on/off the mapping of color scalars through the lookup table.
  // The default is Off. If Off, unsigned char scalars will be used
  // directly as texture. If On, scalars will be mapped through the
  // lookup table to generate 4-component unsigned char scalars.
  // This ivar does not affect other scalars like unsigned short, float,
  // etc. These scalars are always mapped through lookup tables.
  vtkGetMacro(MapColorScalarsThroughLookupTable,int);
  vtkSetMacro(MapColorScalarsThroughLookupTable,int);
  vtkBooleanMacro(MapColorScalarsThroughLookupTable,int);

  // Description:
  // This process object accepts image data as input.
  void SetInput( vtkImageData *input );
  vtkImageData *GetInput();
  
  // Description:
  // Specify the lookup table to convert scalars if necessary
  void SetLookupTable(vtkLookupTable *);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);

  // Description:
  // Get Mapped Scalars
  vtkGetObjectMacro(MappedScalars,vtkUnsignedCharArray);

  // Description:
  // Map scalar values into color scalars.
  unsigned char *MapScalarsToColors (vtkDataArray *scalars);

protected:
  vtkTexture();
  ~vtkTexture();

  int   Repeat;
  int   Interpolate;
  int   Quality;
  int   MapColorScalarsThroughLookupTable;
  vtkLookupTable *LookupTable;
  vtkUnsignedCharArray *MappedScalars;
  
  // this is to duplicated the previous behavior of SelfCreatedLookUpTable
  int SelfAdjustingTableRange;
private:
  vtkTexture(const vtkTexture&);  // Not implemented.
  void operator=(const vtkTexture&);  // Not implemented.
};

#endif
