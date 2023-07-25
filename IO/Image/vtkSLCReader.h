// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkSLCReader : public vtkImageReader2
{
public:
  static vtkSLCReader* New();
  vtkTypeMacro(vtkSLCReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Was there an error on the last read performed?
   */
  vtkGetMacro(Error, int);
  ///@}

  /**
   * Is the given file an SLC file?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;
  /**
   * .slc
   */
  const char* GetFileExtensions() override { return ".slc"; }

  /**
   * SLC
   */
  const char* GetDescriptiveName() override { return "SLC"; }

protected:
  vtkSLCReader();
  ~vtkSLCReader() override;

  // Reads the file name and builds a vtkStructuredPoints dataset.
  void ExecuteDataWithInformation(vtkDataObject*, vtkInformation*) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Decodes an array of eight bit run-length encoded data.
  unsigned char* Decode8BitData(unsigned char* in_ptr, int size);
  int Error;

private:
  vtkSLCReader(const vtkSLCReader&) = delete;
  void operator=(const vtkSLCReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
