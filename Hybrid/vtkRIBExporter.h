/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBExporter.h
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
// .NAME vtkRIBExporter - export a scene into RenderMan RIB format.
// .SECTION Description
// vtkRIBExporter is a concrete subclass of vtkExporter that writes a
// Renderman .RIB files. The input specifies a vtkRenderWindow. All
// visible actors and lights will be included in the rib file. The
// following file naming conventions apply:
//   rib file - FilePrefix.rib
//   image file created by RenderMan - FilePrefix.tif
//   texture files - TexturePrefix_0xADDR_MTIME.tif
// This object does NOT generate an image file. The user must run either
// RenderMan or a RenderMan emulator like Blue Moon Ray Tracer (BMRT).
// vtk properties are convert to Renderman shaders as follows:
//   Normal property, no texture map - plastic.sl
//   Normal property with texture map - txtplastic.sl
// These two shaders must be compiled by the rendering package being
// used.  vtkRIBExporter also supports custom shaders. The shaders are
// written using the Renderman Shading Language. See "The Renderman
// Companion", ISBN 0-201-50868, 1989 for details on writing shaders.
// vtkRIBProperty specifies the declarations and parameter settings for
// custom shaders.
// Tcl Example: generate a rib file for the current rendering.
// vtkRIBExporter myRIB
//   myRIB SetInput $renWin
//   myRIB SetFIlePrefix mine
//   myRIB Write
// This will create a file mine.rib. After running this file through
// a Renderman renderer a file mine.tif will contain the rendered image.
//
// .SECTION See Also
// vtkExporter vtkRIBProperty


#ifndef __vtkRIBExporter_h
#define __vtkRIBExporter_h

#include <stdio.h>
#include "vtkExporter.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkRIBExporter : public vtkExporter
{
public:
  static vtkRIBExporter *New();
  vtkTypeMacro(vtkRIBExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Specify the size of the image for RenderMan. If none is specified, the
  // size of the render window will be used.
  vtkSetVector2Macro(Size,int);
  vtkGetVectorMacro(Size,int,2);

  // Description
  // Specify the sampling rate for the rendering. Default is 2 2.
  vtkSetVector2Macro(PixelSamples,int);
  vtkGetVectorMacro(PixelSamples,int,2);

  // Description:
  // Specify the prefix of the files to write out. The resulting file names
  // will have .RIB appended to them.
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);

  // Description:
  // Specify the prefix of any generated texture files.
  vtkSetStringMacro(TexturePrefix);
  vtkGetStringMacro(TexturePrefix);

  // Description:
  // Set/Get the background flag. Default is 0 (off).
  // If set, the rib file will contain an
  // image shader that will use the renderer window's background
  // color. Normally, RenderMan does generate backgrounds. Backgrounds are
  // composited into the scene with the tiffcomp program that comes with
  // Pixar's RenderMan Toolkit.  In fact, Pixar's Renderman will accept an
  // image shader but only sets the alpha of the background. Images created
  // this way will still have a black background but contain an alpha of 1
  // at all pixels and CANNOT be subsequently composited with other images
  // using tiffcomp.  However, other RenderMan compliant renderers like
  // Blue Moon Ray Tracing (BMRT) do allow image shaders and properly set
  // the background color. If this sounds too confusing, use the following
  // rules: If you are using Pixar's Renderman, leave the Background
  // off. Otherwise, try setting BackGroundOn and see if you get the
  // desired results.
  vtkSetMacro(Background,int);
  vtkGetMacro(Background,int);
  vtkBooleanMacro(Background,int);

protected:
  vtkRIBExporter();
  ~vtkRIBExporter();
  vtkRIBExporter(const vtkRIBExporter&);
  void operator=(const vtkRIBExporter&);

  int Background;
  int Size[2];
  int PixelSamples[2];
  // Description:
  // Write the RIB header.
  void WriteHeader (vtkRenderer *aRen);
  void WriteTrailer ();
  void WriteTexture (vtkTexture *aTexture);
  void WriteViewport (vtkRenderer *aRenderer, int size[2]);
  void WriteCamera (vtkCamera *aCamera);
  void WriteLight (vtkLight *aLight, int count);
  void WriteAmbientLight (int count);
  void WriteProperty (vtkProperty *aProperty, vtkTexture *aTexture);
  void WritePolygons (vtkPolyData *pd, vtkScalars *colors, vtkProperty *aProperty);
  void WriteStrips (vtkPolyData *pd, vtkScalars *colors, vtkProperty *aProperty);

  void WriteData();
  void WriteActor(vtkActor *anActor);
  char *GetTextureName (vtkTexture *aTexture);
  char *GetTIFFName (vtkTexture *aTexture);
  char *FilePrefix;
  FILE *FilePtr;
  char *TexturePrefix;
};

#endif

