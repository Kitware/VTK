// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkJPEGReader : public vtkImageReader2
{
public:
  static vtkJPEGReader* New();
  vtkTypeMacro(vtkJPEGReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Is the given file a JPEG file?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".jpeg .jpg"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "JPEG"; }

protected:
  vtkJPEGReader() = default;
  ~vtkJPEGReader() override = default;

  template <class OT>
  void InternalUpdate(vtkImageData* data, OT* outPtr);

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  vtkJPEGReader(const vtkJPEGReader&) = delete;
  void operator=(const vtkJPEGReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
