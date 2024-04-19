// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMRCReader
 * @brief   read MRC image files
 *
 *
 * A reader to load MRC images.  See http://bio3d.colorado.edu/imod/doc/mrc_format.txt
 * for the file format specification.
 */

#ifndef vtkMRCReader_h
#define vtkMRCReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;

class VTKIOIMAGE_EXPORT vtkMRCReader : public vtkImageAlgorithm
{
public:
  static vtkMRCReader* New();
  vtkTypeMacro(vtkMRCReader, vtkImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  // .Description
  // Get/Set the file to read
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

protected:
  vtkMRCReader();
  ~vtkMRCReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  void ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo) override;

  char* FileName;

private:
  vtkMRCReader(const vtkMRCReader&) = delete;
  void operator=(const vtkMRCReader&) = delete;
  class vtkInternal;
  vtkInternal* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
