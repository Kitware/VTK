/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.h
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
// .NAME vtkImager - Renders into part of a ImageWindow
// .SECTION Description
// vtkImager is the 2D counterpart to vtkRenderer. An Imager renders
// 2D actors into a viewport of an image window. 

// .SECTION See Also
//  vtkImageWindow vtkViewport
   

#ifndef __vtkImager_h
#define __vtkImager_h

#include "vtkObject.h"
#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkViewport.h"


class vtkImageWindow;

class VTK_EXPORT vtkImager : public vtkViewport
{ 
public:
  // Description:
  // Create an imager with viewport (0, 0, 1, 1)
  vtkImager();

  static vtkImager *New() {return new vtkImager;};
  const char *GetClassName() {return "vtkImager";};

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  void Render();

  // Description:
  // Set/Get the image window that an imager is attached to.
  void SetImageWindow (vtkImageWindow* win) 
  {this->VTKWindow = (vtkWindow*) win;};
  vtkImageWindow* GetImageWindow () {return (vtkImageWindow*) this->VTKWindow;};
  void SetVTKWindow (vtkWindow* win) {this->VTKWindow = (vtkWindow*) win;};
  vtkWindow *GetVTKWindow() {return (vtkWindow*) this->VTKWindow;};
  
  // Description:
  // Erase the contents of the imager in the window.
  void Erase(){vtkErrorMacro(<<"vtkImager::Erase - Not implemented!");};

protected:


};


#endif




