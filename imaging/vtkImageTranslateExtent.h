/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTranslateExtent.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageTranslateExtent - Changes extent, nothing else.
// .SECTION Description
// vtkImageTranslateExtent  shift the whole extent, but does not
// change the data.

#ifndef __vtkImageTranslateExtent_h
#define __vtkImageTranslateExtent_h

#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageTranslateExtent : public vtkImageFilter
{
public:
  vtkImageTranslateExtent();
  static vtkImageTranslateExtent *New() {return new vtkImageTranslateExtent;};
  const char *GetClassName() {return "vtkImageTranslateExtent";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Delt to change "WholeExtent". -1 changes 0->10 to -1->9.
  vtkSetVector3Macro(Translation, int);
  vtkGetVector3Macro(Translation, int);

  void InternalUpdate(vtkImageData *outData);

protected:
  int Translation[3];
  
  void ExecuteImageInformation();
};



#endif



