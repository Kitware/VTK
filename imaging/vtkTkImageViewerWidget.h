/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkImageViewerWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkTkImageViewerWidget - a Tk Widget for viewing vtk images

// .SECTION Description
// vtkTkImageViewerWidget is a Tk widget that you can render into. It has a 
// GetImageViewer method that returns a vtkImageViewer. You can also 
// specify a vtkImageViewer to be used when creating the widget by using
// the -iv option. It also takes -width and -height options.


// .SECTION Event Bindings
// Events can be bound on this widget just liek any other Tk widget.

// .SECTION See Also
// vtkImageViewer


#ifndef __vtkTkImageViewerWidget_h
#define __vtkTkImageViewerWidget_h

#include "vtkImageViewer.h"
#include "vtkTclUtil.h"

struct vtkTkImageViewerWidget
{
  Tk_Window  TkWin;		/* Tk window structure */
  Tcl_Interp *Interp;		/* Tcl interpreter */
  int Width;
  int Height;
  vtkImageViewer *ImageViewer;
  char *IV;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

// This widget requires access to structures that are normally 
// not visible to Tcl/Tk applications. For this reason you must
// have access to tkInt.h
#include "tkInt.h"

#ifdef _WIN32
extern "C" {
#include "tkWinInt.h" 
}
#endif

#endif








