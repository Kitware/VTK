/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowLevelLookupTable.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_COMMON_EXPORT vtkWindowLevelLookupTable : public vtkLookupTable
{
public:
  static vtkWindowLevelLookupTable *New();
  vtkTypeRevisionMacro(vtkWindowLevelLookupTable,vtkLookupTable);
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

  float Window;
  float Level;
  int InverseVideo;
  float MaximumTableValue[4];
  float MinimumTableValue[4];
  unsigned char MinimumColor[4];
  unsigned char MaximumColor[4];
private:
  vtkWindowLevelLookupTable(const vtkWindowLevelLookupTable&);  // Not implemented.
  void operator=(const vtkWindowLevelLookupTable&);  // Not implemented.
};

#endif


