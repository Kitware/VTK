/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.h
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
// .NAME vtkImageToStructuredPoints - Attaches image pipeline to VTK. 
// .SECTION Description
// vtkImageToStructuredPoints changes an image cache format to
// a structured points dataset.  It takes an Input plus an optional
// VectorInput. The VectorInput converts the RGB scalar components
// of the VectorInput to vector pointdata attributes. This filter
// will try to reference count the data but in some cases it must
// make a copy.

#ifndef __vtkImageToStructuredPoints_h
#define __vtkImageToStructuredPoints_h

#include "vtkSource.h"
#include "vtkStructuredPoints.h"

class VTK_FILTERING_EXPORT vtkImageToStructuredPoints : public vtkSource
{
public:
  static vtkImageToStructuredPoints *New();
  vtkTypeRevisionMacro(vtkImageToStructuredPoints,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();


  // Description:
  // Set/Get the input object from the image pipeline.
  void SetVectorInput(vtkImageData *input);
  vtkImageData *GetVectorInput();

  // Description:
  // Get the output of this source.
  vtkStructuredPoints *GetOutput();
  vtkStructuredPoints *GetOutput(int idx)
    {return (vtkStructuredPoints *) this->vtkSource::GetOutput(idx); };
  
protected:
  vtkImageToStructuredPoints();
  ~vtkImageToStructuredPoints();

  // to translate the wholeExtent to have min 0 ( I do not like this hack).
  int Translate[3];
  
  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *data);

  
private:
  vtkImageToStructuredPoints(const vtkImageToStructuredPoints&);  // Not implemented.
  void operator=(const vtkImageToStructuredPoints&);  // Not implemented.
};


#endif


