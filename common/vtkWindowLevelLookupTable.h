/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkWindowLevelLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkWindowLevelLookupTable is an object that is used by mapper objects
// to map scalar values into rgba (red-green-blue-alpha transparency)
// color specification, or rgba into scalar values. The color table can
// be created by direct insertion of color values, or by specifying a
// window and level. Window / Level is used in medical imaging to specify
// a lienar greyscale ramp. The Level is the center of the ramp.  The
// Window is the width of the ramp.
// .SECTION Caveats
// vtkWindowLevelLookupTable is a reference counted object. Therefore, you should 
// always use operator "new" to construct new objects. This procedure will
// avoid memory problems (see text).
// .SECTION See Also
// vtkLogLookupTable

#ifndef __vtkWindowLevelLookupTable_h
#define __vtkWindowLevelLookupTable_h

#include "vtkRefCount.h"
#include "vtkAPixmap.h"
#include "vtkLookupTable.h"

class VTK_EXPORT vtkWindowLevelLookupTable : public vtkLookupTable
{
public:
  vtkWindowLevelLookupTable(int sze=256, int ext=256);
  void Build();
  vtkWindowLevelLookupTable *New() {return new vtkWindowLevelLookupTable;};
  char *GetClassName() {return "vtkWindowLevelLookupTable";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Window for the lookuptable. Window is the width of the lookuptable
  // ramp.
  vtkSetClampMacro(Window,float,1.0,65536.0);
  vtkGetMacro(Window,float);

  // Description:
  // Set the Level for the lookuptable. Level is the center of the ramp of the
  // lookuptable.
  // ramp.
  vtkSetMacro(Level,float);
  vtkGetMacro(Level,float);

  // Description:
  // Set inverse video on or off.
  vtkSetMacro(InverseVideo,int);
  vtkGetMacro(InverseVideo,int);
  vtkBooleanMacro(InverseVideo,int);

  // Description:
  // Set the Minimum color. All lookup table entries below the start of the ramp
  // will be set to this color.
  vtkSetVector4Macro(MinimumColor,unsigned char);
  vtkGetVectorMacro(MinimumColor,unsigned char,4);

  // Description:
  // Set the Maximum color. All lookup table entries above the end of the ramp
  // will be set to this color.
  vtkSetVector4Macro(MaximumColor,unsigned char);
  vtkGetVectorMacro(MaximumColor,unsigned char,4);

protected:
  float Window;
  float Level;
  int MapScalarToIndex (float scalar);
  int InverseVideo;
  unsigned char MinimumColor[4];
  unsigned char MaximumColor[4];
};

#endif


