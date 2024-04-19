// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTGAReader
 * @brief   read PNG files
 *
 * vtkTGAReader is a source object that reads Targa files.
 * It supports uncompressed 24 and 32 bits formats.
 *
 * @sa
 * vtkImageReader2
 */

#ifndef vtkTGAReader_h
#define vtkTGAReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkTGAReader : public vtkImageReader2
{
public:
  static vtkTGAReader* New();
  vtkTypeMacro(vtkTGAReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Is the given file a valid TGA file?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".tga"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Targa"; }

protected:
  vtkTGAReader() = default;
  ~vtkTGAReader() override = default;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo) override;

private:
  vtkTGAReader(const vtkTGAReader&) = delete;
  void operator=(const vtkTGAReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
