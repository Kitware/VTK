/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXTextMapper.h
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
// .NAME vtkXTextMapper - 2D Text annotation support for X
// .SECTION Description
// vtkXTextMapper provides 2D text annotation support for vtx under Xwindows.
// Normally the user should use vtktextMapper which in turn will use
// this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkXTextMapper_h
#define __vtkXTextMapper_h

#include "vtkTextMapper.h"

#include        <X11/Xlib.h>
#include        <X11/Xutil.h>
#include        <X11/cursorfont.h>
#include        <X11/X.h>
#include        <X11/keysym.h>


class VTK_EXPORT vtkXTextMapper : public vtkTextMapper
{
public:
  const char *GetClassName() {return "vtkXTextMapper";};
  static vtkXTextMapper *New() {return new vtkXTextMapper;};

  // Description:
  // Set the font size used by the mapper.  If the font size is 
  // available, the code will use the nearest available size.
  void SetFontSize(int size);

  // Description:
  // Return the X compositing mode being used.
  int GetCompositingMode(vtkActor2D* actor);

  // Description:
  // Draw the text to the screen.
  void Render(vtkViewport* viewport, vtkActor2D* actor);

protected:

};




#endif



