/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.h
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
// .NAME vtkRenderLargeImage - Use tiling to generate a large rendering
// .SECTION Description
// vtkRenderLargeImage provides methods needed to read a region from a file.


#ifndef __vtkRenderLargeImage_h
#define __vtkRenderLargeImage_h

#include <iostream.h>
#include <fstream.h>
#include "vtkImageSource.h"

class vtkRenderer;

class VTK_EXPORT vtkRenderLargeImage : public vtkImageSource
{
public:
  vtkRenderLargeImage();
  static vtkRenderLargeImage *New() {return new vtkRenderLargeImage;};
  const char *GetClassName() {return "vtkRenderLargeImage";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // The magnification of the current render window
  vtkSetMacro(Magnification,int);
  vtkGetMacro(Magnification,int);

  // Description:
  // Indicates what renderer to get the pixel data from.
  vtkSetObjectMacro(Input,vtkRenderer);

  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkRenderer);

  void UpdateImageInformation();
  
protected:
  int Magnification;
  vtkRenderer *Input;
  void Execute(vtkImageData *data);
};

#endif
