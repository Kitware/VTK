/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.h
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
// .NAME vtkImageDataStreamer - Initiates streaming on image data.
// .SECTION Description
// To satisfy a request, this filter calls update on its input
// many times with smaller update extents.  All processing up stream
// streams smaller pieces.

#ifndef __vtkImageDataStreamer_h
#define __vtkImageDataStreamer_h

#include "vtkImageToImageFilter.h"
#include "vtkExtentTranslator.h"

class VTK_IMAGING_EXPORT vtkImageDataStreamer : public vtkImageToImageFilter
{
public:
  static vtkImageDataStreamer *New();
  vtkTypeRevisionMacro(vtkImageDataStreamer,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set how many pieces to divide the input into.
  // void SetNumberOfStreamDivisions(int num);
  // int GetNumberOfStreamDivisions();
  vtkSetMacro(NumberOfStreamDivisions,int);
  vtkGetMacro(NumberOfStreamDivisions,int);
  
  // Description:
  // Need to override since this is where streaming will be done
  void UpdateData( vtkDataObject *out );

  // Description:
  // Get the extent translator that will be used to split the requests
  vtkGetObjectMacro(ExtentTranslator,vtkExtentTranslator);
  vtkSetObjectMacro(ExtentTranslator,vtkExtentTranslator);
  
protected:
  vtkImageDataStreamer();
  ~vtkImageDataStreamer();
  
  vtkExtentTranslator *ExtentTranslator;
  int            NumberOfStreamDivisions;
private:
  vtkImageDataStreamer(const vtkImageDataStreamer&);  // Not implemented.
  void operator=(const vtkImageDataStreamer&);  // Not implemented.
};




#endif



