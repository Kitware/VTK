/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.h
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
// .NAME vtkOutputWindow - base class for writting debug output to a console
// .SECTION Description
// This class is used to encapsulate all text ouput, so that it will work
// with operating systems that have a stdout and stderr, and ones that
// do not.  (i.e windows does not).  Sub-classes can be provided which can
// redirect the output to a window.

#ifndef __vtkOutputWindow_h
#define __vtkOutputWindow_h

#include "vtkObject.h"


class VTK_EXPORT vtkOutputWindow : public vtkObject
{
public: 
// Methods from vtkObject
  virtual const char *GetClassName() {return "vtkOutputWindow";};
  // Description:
  // Print ObjectFactor to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is a singleton pattern New.  There will only be ONE
  // reference to a vtkOutputWindow object per process.  Clients that
  // call this must call Delete on the object so that the reference 
  // counting will work.   The single instance will be unreferenced when
  // the program exits.
  static vtkOutputWindow* New();
  // Description:
  // Return the singleton instance with no reference counting.
  static vtkOutputWindow* GetInstance();
  virtual void DisplayText(const char*);
private:
  static vtkOutputWindow* Instance;
};

#endif
