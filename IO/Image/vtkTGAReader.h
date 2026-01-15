// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTGAReader
 * @brief   read PNG files
 *
 * vtkTGAReader is a source object that reads Targa files.
 * It supports uncompressed 24 and 32 bits formats.
 * This reader supports the stream API.
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

  ///@{
  /**
   * Return 1 if, after a quick check of file header, it looks like the provided file or stream
   * can be read as a tga file. Return 0 if it is sure it cannot be read. The stream version may
   * move the stream cursor. This only checks the header can be read and the second byte of the
   * header contain either the code for compressed or uncompressed RGB images which are the only
   * supported formats. Please note any binary data could make this method return 1 if they satisfy
   * above condition.
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;
  int CanReadFile(vtkResourceStream* stream) override;
  ///@}

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
