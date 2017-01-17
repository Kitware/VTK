/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLCReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSLCReader
 * @brief   read an SLC volume file.
 *
 * vtkSLCReader reads an SLC file and creates a structured point dataset.
 * The size of the volume and the data spacing is set from the SLC file
 * header.
*/

#ifndef vtkSLCReader_h
#define vtkSLCReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class VTKIOIMAGE_EXPORT vtkSLCReader : public vtkImageReader2
{
public:
  static vtkSLCReader *New();
  vtkTypeMacro(vtkSLCReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Was there an error on the last read performed?
   */
  vtkGetMacro(Error,int);
  //@}

  /**
   * Is the given file an SLC file?
   */
  int CanReadFile(const char* fname) VTK_OVERRIDE;
  /**
   * .slc
   */
  const char* GetFileExtensions() VTK_OVERRIDE
  {
      return ".slc";
  }

  /**
   * SLC
   */
  const char* GetDescriptiveName() VTK_OVERRIDE
  {
      return "SLC";
  }

protected:
  vtkSLCReader();
  ~vtkSLCReader() VTK_OVERRIDE;

  // Reads the file name and builds a vtkStructuredPoints dataset.
  void ExecuteDataWithInformation(vtkDataObject*, vtkInformation*) VTK_OVERRIDE;

  int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector) VTK_OVERRIDE;

  // Decodes an array of eight bit run-length encoded data.
  unsigned char *Decode8BitData( unsigned char *in_ptr, int size );
  int Error;
private:
  vtkSLCReader(const vtkSLCReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSLCReader&) VTK_DELETE_FUNCTION;
};

#endif


