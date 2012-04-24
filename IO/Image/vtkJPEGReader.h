/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkJPEGReader - read JPEG files
// .SECTION Description
// vtkJPEGReader is a source object that reads JPEG files.
// It should be able to read most any JPEG file
//
// .SECTION See Also
// vtkJPEGWriter

#ifndef __vtkJPEGReader_h
#define __vtkJPEGReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class VTKIOIMAGE_EXPORT vtkJPEGReader : public vtkImageReader2
{
public:
  static vtkJPEGReader *New();
  vtkTypeMacro(vtkJPEGReader,vtkImageReader2);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the given file a JPEG file?
  int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
      return ".jpeg .jpg";
    }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
      return "JPEG";
    }
protected:
  vtkJPEGReader() {};
  ~vtkJPEGReader() {};

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);
private:
  vtkJPEGReader(const vtkJPEGReader&);  // Not implemented.
  void operator=(const vtkJPEGReader&);  // Not implemented.
};
#endif


