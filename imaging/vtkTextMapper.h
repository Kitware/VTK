/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkTextMapper - 2D Text annotation
// .SECTION Description
// vtkTextMapper provides 2D text annotation support for vtk.
// It is a Mapper2D that can be accosciated with a Actor2D
// and placed withint a RenderWindow or ImageWindow.

// .SECTION See Also
// vtkMapper2D vtkActor2D

#ifndef __vtkTextMapper_h
#define __vtkTextMapper_h


#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"

#define VTK_ARIAL     0
#define VTK_COURIER   1
#define VTK_TIMES     2

class VTK_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTextMapper();
  const char *GetClassName() {return "vtkTextMapper";};
  static vtkTextMapper *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Draw the text to the screen.  This function is implemented in
  // the subclasses.
  virtual void Render(vtkViewport* viewport, vtkActor2D* actor) = 0;

  // Description:
  // Set the input to the mapper.  The mapper doesn't parse the string
  // for carriage returns or line feeds.
  vtkSetStringMacro(Input);

  // Description:
  // Set the font size used by the mapper.  The subclasses can override
  // this function since all font sizes may not be available (especially
  // in X).
  virtual void SetFontSize(int size) 
  {this->FontSize = size; this->FontChanged = 1; this->Modified();};

  // Description:
  // Return the font size actually in use by the mapper.  This value may
  // not match the value specified in the last SetFontSize if the last size
  // was unavailable.
  vtkGetMacro(FontSize, int);

  // Description:
  // Set/Get the bold property.
  //  vtkSetMacro(Bold, int);
  void SetBold(int val) 
  {this->Bold = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Set/Get the italic property.
  // vtkSetMacro(Italic, int);
  void SetItalic(int val) 
  {this->Italic = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Set/Get the font family.  Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  // vtkSetMacro(FontFamily, int);
  void SetFontFamily(int val) 
  {this->FontFamily = val; this->FontChanged = 1; this->Modified();};
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {SetFontFamily(VTK_TIMES);};

protected:
  int   Italic;
  int	Bold;
  int   FontSize;
  int   FontFamily;
  char* Input;
  int   FontChanged;  
};


#endif

