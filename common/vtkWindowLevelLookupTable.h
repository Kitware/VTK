/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

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
// .NAME vtkWindowLevelLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkWindowLevelLookupTable is an object that is used by mapper objects
// to map scalar values into rgba (red-green-blue-alpha transparency)
// color specification, or rgba into scalar values. The color table can
// be created by direct insertion of color values, or by specifying a
// window and level. Window / Level is used in medical imaging to specify
// a linear greyscale ramp. The Level is the center of the ramp.  The
// Window is the width of the ramp.

// .SECTION Caveats
// vtkWindowLevelLookupTable is a reference counted object. Therefore, you
// should always use operator "new" to construct new objects. This procedure
// will avoid memory problems (see text).

// .SECTION See Also
// vtkLogLookupTable

#ifndef __vtkWindowLevelLookupTable_h
#define __vtkWindowLevelLookupTable_h

#include "vtkObject.h"
#include "vtkLookupTable.h"

class VTK_EXPORT vtkWindowLevelLookupTable : public vtkLookupTable
{
public:
  static vtkWindowLevelLookupTable *New();
  vtkTypeMacro(vtkWindowLevelLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generate lookup table as a linear ramp between MinimumTableValue
  // and MaximumTableValue.
  void Build();

  // Description:
  // Set the window for the lookup table.  The window is the difference
  // between TableRange[0] and TableRange[1].
  void SetWindow(float window) {
    if (window < 1e-5) { window = 1e-5; }
    this->Window = window;
    this->SetTableRange(this->Level - this->Window/2.0,
			this->Level + this->Window/2.0); };
  vtkGetMacro(Window,float);

  // Description:
  // Set the Level for the lookup table.  The level is the average of
  // TableRange[0] and TableRange[1].
  void SetLevel(float level) {
    this->Level = level;
    this->SetTableRange(this->Level - this->Window/2.0,
			this->Level + this->Window/2.0); };
  vtkGetMacro(Level,float);

  // Description:
  // Set inverse video on or off.  You can achieve the same effect by
  // switching the MinimumTableValue and the MaximumTableValue.
  void SetInverseVideo(int iv);
  vtkGetMacro(InverseVideo,int);
  vtkBooleanMacro(InverseVideo,int);

  // Description:
  // Set the minimum table value.  All lookup table entries below the
  // start of the ramp will be set to this color.  After you change
  // this value, you must re-build the lookup table.
  vtkSetVector4Macro(MinimumTableValue,float);
  vtkGetVector4Macro(MinimumTableValue,float);

  // Description:
  // Set the maximum table value. All lookup table entries above the
  // end of the ramp will be set to this color.  After you change
  // this value, you must re-build the lookup table.
  vtkSetVector4Macro(MaximumTableValue,float);
  vtkGetVector4Macro(MaximumTableValue,float);

  // Description:
  // For backwards compatibility: specify the color using integers
  // in the range [0,255].  Deprecated: use SetMinimumTableValue()
  // instead.
  void SetMinimumColor(int r, int g, int b, int a) {
    this->SetMinimumTableValue(r*255.0,g*255.0,b*255.0,a*255.0); };
  void SetMinimumColor(const unsigned char rgba[4]) {
    this->SetMinimumColor(rgba[0],rgba[1],rgba[2],rgba[3]); };
  void GetMinimumColor(unsigned char rgba[4]) {
    rgba[0] = int(this->MinimumColor[0]*255);
    rgba[1] = int(this->MinimumColor[1]*255);
    rgba[2] = int(this->MinimumColor[2]*255);
    rgba[3] = int(this->MinimumColor[3]*255); };
  unsigned char *GetMinimumColor() {
    this->GetMinimumColor(this->MinimumColor); 
    return this->MinimumColor; };

  // Description:
  // For backwards compatibility: specify the color using integers
  // in the range [0,255].  Deprecated: use SetMaximumTableValue()
  // instead.
  void SetMaximumColor(int r, int g, int b, int a) {
    this->SetMaximumTableValue(r*255.0,g*255.0,b*255.0,a*255.0); };
  void SetMaximumColor(const unsigned char rgba[4]) {
    this->SetMaximumColor(rgba[0],rgba[1],rgba[2],rgba[3]); };
  void GetMaximumColor(unsigned char rgba[4]) {
    rgba[0] = int(this->MaximumColor[0]*255);
    rgba[1] = int(this->MaximumColor[1]*255);
    rgba[2] = int(this->MaximumColor[2]*255);
    rgba[3] = int(this->MaximumColor[3]*255); };
  unsigned char *GetMaximumColor() {
    this->GetMaximumColor(this->MaximumColor); 
    return this->MaximumColor; };

protected:
  vtkWindowLevelLookupTable(int sze=256, int ext=256);
  ~vtkWindowLevelLookupTable() {};
  vtkWindowLevelLookupTable(const vtkWindowLevelLookupTable&) {};
  void operator=(const vtkWindowLevelLookupTable&) {};

  float Window;
  float Level;
  int InverseVideo;
  float MaximumTableValue[4];
  float MinimumTableValue[4];
  unsigned char MinimumColor[4];
  unsigned char MaximumColor[4];
};

#endif


