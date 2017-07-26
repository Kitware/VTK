/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegY2DReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSegY2DReader_h
#define __vtkSegY2DReader_h

#include "vtkSegY2DReader.h"

#include "vtkSegYReader.h"
#include "vtkStructuredGridAlgorithm.h"

#include <vtkIOSegYModule.h> // For export macro

class VTKIOSEGY_EXPORT vtkSegY2DReader : public vtkStructuredGridAlgorithm
{
public:
  static vtkSegY2DReader* New();
  vtkTypeMacro(vtkSegY2DReader, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetStringMacro(FileName);

  bool GetImageData(vtkImageData* imageData)
  {
    return reader.GetImageData(imageData);
  }

  vtkSegY2DReader();
  ~vtkSegY2DReader();

protected:
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  char* FileName;
  vtkSegYReader reader;

private:
  vtkSegY2DReader(const vtkSegY2DReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSegY2DReader&) VTK_DELETE_FUNCTION;
};

#endif // __vtkSegY2DReader_h
