/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkScalarBarActor - Create a scalar bar with labels
// .SECTION Description
// vtkScalarBarActor creates a scalar bar with annotation text. A scalar
// bar is a legend that indicates to the viewer the correspondance between
// color value and data value. The legend consists of a rectangular bar 
// made of rectangular pieces each colored a constant value. Since 
// vtkScalarBarActor is a subclass of vtkActor2D, it is drawn in the image 
// plane (i.e., in the renderer's viewport) on top of the 3D graphics window.
//
// To use vtkScalarBarActor you must associate a vtkLookupTable (or
// subclass) with it. The lookup table defines the colors and the
// range of scalar values used to map scalar data.  Typically, the
// number of colors shown in the scalar bar is not equal to the number
// of colors in the lookup table, in which case sampling of
// the lookup table is performed. 
//
// Other optional capibilities include specifying the fraction of the
// viewport size (both x and y directions) which will control the size
// of the scalar bar, the number of annotation labels, and the font
// attributes of the annotation text. The actual position of the
// scalar bar on the screen is controlled by using the
// vtkActor2D::SetPosition() method (by default the scalar bar is
// centered in the viewport).  Other features include the ability to
// orient the scalar bar horizontally of vertically and controlling
// the format (printf style) with which to print the labels on the
// scalar bar. Also, the vtkScalarBarActor's property is applied to
// the scalar bar and annotation (including color, layer, and
// compositing operator).

// .SECTION See Also
// vtkActor2D vtkTextMapper vtkPolyDataMapper2D

#ifndef __vtkScalarBarActor_h
#define __vtkScalarBarActor_h

#include "vtkActor2D.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"

#define VTK_ORIENT_HORIZONTAL 0
#define VTK_ORIENT_VERTICAL 1

class VTK_EXPORT vtkScalarBarActor : public vtkActor2D
{
public:
  vtkScalarBarActor();
  ~vtkScalarBarActor();
  const char *GetClassName() {return "vtkScalarBarActor";};
  static vtkScalarBarActor *New() {return new vtkScalarBarActor;};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Draw the scalar bar and annotation text to the screen.
  void Render(vtkViewport*);

  // Description:
  // Set/Get the vtkMapper which contains the lookup table and scalar
  // data from which to create the scalar bar actor. The lookup table
  // specifies the number of colors to use in the table (if not
  // overridden), as well as the scalar range.
  vtkSetObjectMacro(LookupTable,vtkLookupTable);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);

  // Description:
  // 
  // Set/Get the width of the scalar bar. The value is expressed
  // as a fraction of the viewport. (Note: if the orientation of the scalar
  // bar is vertical, then the width is in the direction of the viewport 
  // x-axis. If the orientation is horizontal, the width is in the direction 
  // of the  viewport y-axis.)
  vtkSetClampMacro(Width, float, 0.0, 1.0);
  vtkGetMacro(Width, float);
  
  // Description:
  // Set/Get the height of the scalar bar. The value is expressed
  // as a fraction of the viewport. (Note: if the orientation of the scalar
  // bar is vertical, then the height is in the direction of the viewport 
  // y-axis. If the orientation is horizontal, the height is in the direction 
  // of the  viewport x-axis.)
  vtkSetClampMacro(Height, float, 0.0, 1.0);
  vtkGetMacro(Height, float);
  
  // Description:
  // Set/Get the maximum number of scalar bar segements to show. This may
  // differ from the number of colors in the lookup table, in which case
  // the colors are samples from the lookup table.
  vtkSetClampMacro(MaximumNumberOfColors, int, 2, VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfColors, int);
  
  // Description:
  // Set/Get the number of annotation labels to show.
  vtkSetClampMacro(NumberOfLabels, int, 0, 64);
  vtkGetMacro(NumberOfLabels, int);
  
  // Description:
  // Control the orientation of the scalar bar.
  vtkSetClampMacro(Orientation,int,VTK_ORIENT_HORIZONTAL, VTK_ORIENT_VERTICAL);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal()
       {this->SetOrientation(VTK_ORIENT_HORIZONTAL);};
  void SetOrientationToVertical() {this->SetOrientation(VTK_ORIENT_VERTICAL);};

  // Description:
  // Set/Get suggested font size used to annotate the scalar bar. (Suggested
  // because not all font sizes may be available.) Value is expressed in points
  vtkSetClampMacro(FontSize,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(FontSize,int);

  // Description:
  // Enable/Disable bolding annotation text.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/Disable italicizing annotation text.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/Disable creating shadows on the annotation text. Shadows make 
  // the text easier to read.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the font family for the annotation text. Three font types 
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and 
  // Times (VTK_TIMES).
  vtkSetMacro(FontFamily, int);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

  // Description:
  // Set/Get the format with which to print the labels on the scalar
  // bar.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Set/Get the title of the scalar bar actor,
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

protected:
  vtkLookupTable *LookupTable;
  float Width;
  float Height;
  int   MaximumNumberOfColors;
  int   NumberOfLabels;
  int   NumberOfLabelsBuilt;
  int   Orientation;
  char  *Title;

  int   FontSize;
  int	Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  char  *LabelFormat;

private:
  vtkTextMapper *TitleMapper;
  vtkActor2D    *TitleActor;

  vtkTextMapper **TextMappers;
  vtkActor2D    **TextActors;

  vtkPolyData         *ScalarBar;
  vtkPolyDataMapper2D *ScalarBarMapper;
  vtkActor2D          *ScalarBarActor;

  vtkTimeStamp  BuildTime;
};


#endif

