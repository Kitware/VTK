/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkRendererSource - read pnm (i.e., portable anymap) files
// .SECTION Description
// vtkRendererSource is a source object that reads pnm (portable anymap) files.
// This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
// (Currently this object only reads binary versions of these files).
//    PNMSource creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//    To read a volume, files must be of the form "filename.<number>"
// (e.g., foo.ppm.0, foo.ppm.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers). 
//    The default behavior is to read a single file. In this case, the form
// of the file is simply "filename" (e.g., foo.bar, foo.ppm, foo.pnm). To 
// differentiate between reading images and volumes, the image range is set
// to  (-1,-1) to read a single image file.

#ifndef __vtkRendererSource_hh
#define __vtkRendererSource_hh

#include "SPtsSrc.hh"
#include "Renderer.hh"

class vtkRendererSource : public vtkStructuredPointsSource
{
public:
  vtkRendererSource();
  ~vtkRendererSource() {};
  char *GetClassName() {return "vtkRendererSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Indicates what renderer to get the pixel data from.
  vtkSetObjectMacro(Input,vtkRenderer);
  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkRenderer);

protected:
  void Execute();
  vtkRenderer *Input;
};

#endif


