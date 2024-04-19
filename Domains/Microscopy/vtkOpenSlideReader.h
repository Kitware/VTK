// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenSlideReader
 * @brief   read digital whole slide images supported by
 * openslide library
 *
 * vtkOpenSlideReader is a source object that uses openslide library to
 * read multiple supported image formats used for whole slide images in
 * microscopy community.
 *
 * @sa
 * vtkPTIFWriter
 */

#ifndef vtkOpenSlideReader_h
#define vtkOpenSlideReader_h

#include "vtkDomainsMicroscopyModule.h" // For export macro
#include "vtkImageReader2.h"

extern "C"
{
#include "openslide/openslide.h" // For openslide support
}

VTK_ABI_NAMESPACE_BEGIN
class VTKDOMAINSMICROSCOPY_EXPORT vtkOpenSlideReader : public vtkImageReader2
{
public:
  static vtkOpenSlideReader* New();
  vtkTypeMacro(vtkOpenSlideReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Is the given file supported ?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override
  {
    return ".ndpi .svs"; // TODO: Get exhaustive list of formats
  }

  ///@{
  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Openslide::WholeSlideImage"; }

protected:
  vtkOpenSlideReader() = default;
  ~vtkOpenSlideReader() override;
  ///@}

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  openslide_t* openslide_handle = nullptr;

  vtkOpenSlideReader(const vtkOpenSlideReader&) = delete;
  void operator=(const vtkOpenSlideReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
