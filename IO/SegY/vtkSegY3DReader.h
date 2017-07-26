/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegY3DReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSegY3DReader_h
#define __vtkSegY3DReader_h

#include "vtkImageAlgorithm.h"
#include "vtkSegYReader.h"
#include "vtkSmartPointer.h"

#include <vtkIOSegYModule.h> // For export macro

// Forward declarations
class vtkImageData;

class VTKIOSEGY_EXPORT vtkSegY3DReader : public vtkImageAlgorithm
{
public:
  static vtkSegY3DReader* New();

  vtkTypeMacro(vtkSegY3DReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  virtual vtkImageData* GetImage(int ImageNumber);

protected:
  vtkSegY3DReader();
  ~vtkSegY3DReader();

  char* FileName;
  vtkSegYReader reader;
  vtkSmartPointer<vtkImageData> image;

private:
  vtkSegY3DReader(const vtkSegY3DReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSegY3DReader&) VTK_DELETE_FUNCTION;
};

#endif // __vtkSegY3DReader_h
