/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.h
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
// .NAME vtkWindowToImageFilter - Use a vtkWindow as input to image pipeline
// .SECTION Description
// vtkWindowToImageFilter provides methods needed to read the data in a 
// vtkWindow and use it as input to the imaging pipeline. This is
// useful for saving an image to a file for example. Use this filter to
// convert RenderWindows or ImageWindows to an image format.

#ifndef __vtkWindowToImageFilter_h
#define __vtkWindowToImageFilter_h

#include "vtkImageSource.h"

class vtkWindow;

class VTK_RENDERING_EXPORT vtkWindowToImageFilter : public vtkImageSource
{
public:
  static vtkWindowToImageFilter *New();

  vtkTypeRevisionMacro(vtkWindowToImageFilter,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Indicates what renderer to get the pixel data from.
  void SetInput(vtkWindow *input);

  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkWindow);

  // Description:
  // The magnification of the current render window
  vtkSetClampMacro(Magnification,int,1,2048);
  vtkGetMacro(Magnification,int);

protected:
  vtkWindowToImageFilter();
  ~vtkWindowToImageFilter();

  // vtkWindow is not a vtkDataObject, so we need our own ivar.
  vtkWindow *Input;
  int Magnification;
  void ExecuteInformation();
  void ExecuteData(vtkDataObject *data);
private:
  vtkWindowToImageFilter(const vtkWindowToImageFilter&);  // Not implemented.
  void operator=(const vtkWindowToImageFilter&);  // Not implemented.
};

#endif
