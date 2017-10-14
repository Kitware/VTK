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
/**
 * @class   vtkJPEGReader
 * @brief   read JPEG files
 *
 * vtkJPEGReader is a source object that reads JPEG files.
 * The reader can also read an image from a memory buffer,
 * see vtkImageReader2::MemoryBuffer.
 * It should be able to read most any JPEG file.
 *
 * @sa
 * vtkJPEGWriter
*/

#ifndef vtkJPEGReader_h
#define vtkJPEGReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class VTKIOIMAGE_EXPORT vtkJPEGReader : public vtkImageReader2
{
public:
  static vtkJPEGReader *New();
  vtkTypeMacro(vtkJPEGReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Is the given file a JPEG file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override
  {
      return ".jpeg .jpg";
  }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override
  {
      return "JPEG";
  }
protected:
  vtkJPEGReader() {}
  ~vtkJPEGReader() override {}

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo) override;
private:
  vtkJPEGReader(const vtkJPEGReader&) = delete;
  void operator=(const vtkJPEGReader&) = delete;
};
#endif


