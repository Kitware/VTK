/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.h
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
// .NAME vtkRendererSource - take a renderer into the pipeline
// .SECTION Description
// vtkRendererSource is a source object that gets its input from a 
// renderer and converts it to structured points. This can then be 
// used in a visualization pipeline. You must explicitly send a 
// Modify() to this object to get it to reload its data from the
// renderer. Consider using vtkWindowToImageFilter instead of this
// class.
//
// The data placed into the output is the renderer's image rgb values.
// Optionally, you can also grab the image depth (e.g., z-buffer) values, and
// place then into the output (point) field data.

// .SECTION see also
// vtkWindowToImageFilter vtkRenderer vtkStructuredPoints

#ifndef __vtkRendererSource_h
#define __vtkRendererSource_h

#include "vtkStructuredPointsSource.h"
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkRendererSource : public vtkStructuredPointsSource
{
public:
  static vtkRendererSource *New();
  vtkTypeRevisionMacro(vtkRendererSource,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime also considering the Renderer.
  unsigned long GetMTime();

  // Description:
  // Indicates what renderer to get the pixel data from.
  vtkSetObjectMacro(Input,vtkRenderer);

  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkRenderer);

  // Description:
  // Use the entire RenderWindow as a data source or just the Renderer.
  // The default is zero, just the Renderer.
  vtkSetMacro(WholeWindow,int);
  vtkGetMacro(WholeWindow,int);
  vtkBooleanMacro(WholeWindow,int);
  
  // Description:
  // If this flag is on, the Executing causes a render first.
  vtkSetMacro(RenderFlag, int);
  vtkGetMacro(RenderFlag, int);
  vtkBooleanMacro(RenderFlag, int);

  // Description:
  // A boolean value to control whether to grab z-buffer 
  // (i.e., depth values) along with the image data. The z-buffer data
  // is placed into the field data attributes.
  vtkSetMacro(DepthValues,int);
  vtkGetMacro(DepthValues,int);
  vtkBooleanMacro(DepthValues,int);
  
protected:
  vtkRendererSource();
  ~vtkRendererSource();

  void Execute();
  
  void UpdateInformation();
  
  vtkRenderer *Input;
  int WholeWindow;
  int RenderFlag;
  int DepthValues;

private:
  vtkRendererSource(const vtkRendererSource&);  // Not implemented.
  void operator=(const vtkRendererSource&);  // Not implemented.
};

#endif


