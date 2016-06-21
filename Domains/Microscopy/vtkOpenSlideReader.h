/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenSlideReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenSlideReader - read digital whole slide images supported by
// openslide library
// .SECTION Description
// vtkOpenSlideReader is a source object that uses openslide library to
// read multiple supported image formats used for whole slide images in
// microscopy community.
//
// .SECTION See Also
// vtkPTIFWriter

#ifndef vtkOpenSlideReader_h
#define vtkOpenSlideReader_h

#include "vtkDomainsMicroscopyModule.h" // For export macro
#include "vtkImageReader2.h"

extern "C" {
#include "openslide/openslide.h" // For openslide support
}

class VTKDOMAINSMICROSCOPY_EXPORT vtkOpenSlideReader : public vtkImageReader2
{
public:
  static vtkOpenSlideReader *New();
  vtkTypeMacro(vtkOpenSlideReader,vtkImageReader2);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the given file supported ?
  int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
      return ".ndpi .svs"; // TODO: Get exaustive list of formats
    }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
      return "Openslide::WholeSlideImage";
    }
protected:
  vtkOpenSlideReader() {}
  ~vtkOpenSlideReader();

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);
private:
  openslide_t *openslide_handle;

  vtkOpenSlideReader(const vtkOpenSlideReader&);  // Not implemented.
  void operator=(const vtkOpenSlideReader&) VTK_DELETE_FUNCTION;
};
#endif
