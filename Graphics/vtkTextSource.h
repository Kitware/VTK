/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextSource.h
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
// .NAME vtkTextSource - create polygonal text
// .SECTION Description
// vtkTextSource converts a text string into polygons.  This way you can 
// insert text into your renderings. It uses the 9x15 font from X Windows.
// You can specify if you want the background to be drawn or not. The
// characters are formed by scan converting the raster font into
// quadrilaterals. Colors are assigned to the letters using scalar data.
// To set the color of the characters with the source's actor property, set
// BackingOff on the text source and ScalarVisibilityOff on the associated
// vtkPolyDataMapper. Then, the color can be set using the associated actor's
// property.
//
// vtkVectorText generates higher quality polygonal representations of
// characters.

// .SECTION See Also
// vtkVectorText

#ifndef __vtkTextSource_h
#define __vtkTextSource_h

#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkTextSource : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkTextSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with no string set and backing enabled.
  static vtkTextSource *New();

  // Description:
  // Set/Get the text to be drawn.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  // Description:
  // Controls whether or not a background is drawn with the text.
  vtkSetMacro(Backing,int);
  vtkGetMacro(Backing,int);
  vtkBooleanMacro(Backing,int);

  // Description:
  // Set/Get the foreground color. Default is white (1,1,1). ALpha is always 1.
  vtkSetVector3Macro(ForegroundColor,float);
  vtkGetVectorMacro(ForegroundColor,float,3);

  // Description:
  // Set/Get the background color. Default is black (0,0,0). Alpha is always 1.
  vtkSetVector3Macro(BackgroundColor,float);
  vtkGetVectorMacro(BackgroundColor,float,3);

protected:
  vtkTextSource();
  ~vtkTextSource();
  vtkTextSource(const vtkTextSource&);
  void operator=(const vtkTextSource&);

  void Execute();
  char *Text;
  int  Backing;
  float ForegroundColor[4];
  float BackgroundColor[4];
};

#endif


