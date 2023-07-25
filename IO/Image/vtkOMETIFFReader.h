// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOMETIFFReader
 * @brief reader for OME TIFF files
 *
 * vtkOMETIFFReader supports reading OME-TIFF files. These are TIFF files with
 * OME meta-data that helps interpret the contents of the TIFF.
 *
 * The OME-TIFF specification is available here:
 * https://docs.openmicroscopy.org/ome-model/5.6.3/ome-tiff/specification.html#ome-tiff-specification
 *
 * The current implementation only supports single-file, multi-page TIFF. It
 * will not read multi-file OME-TIFF files correctly.
 *
 * Unlike most image readers, this reader does not support arbitrary sub-extent
 * requests. This is because the splicing of the `z`, `t`, and `c` planes can make it
 * tricky to read sub-extents in `z` for certain dimension orders. This reader
 * supports piece-request instead and satisfies such request by splitting the
 * `XY` plane into requested number of pieces.
 *
 * The reader lets the superclass read the whole TIFF volume and then splice it
 * up into channels, timesteps, and z-planes. The parts are then cached
 * internally so that subsequent timestep requests can be served without
 * re-reading the file.
 */

#ifndef vtkOMETIFFReader_h
#define vtkOMETIFFReader_h

#include "vtkTIFFReader.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkOMETIFFReader : public vtkTIFFReader
{
public:
  static vtkOMETIFFReader* New();
  vtkTypeMacro(vtkOMETIFFReader, vtkTIFFReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  int CanReadFile(VTK_FILEPATH const char* fname) override;
  const char* GetFileExtensions() override { return ".ome.tif .ome.tiff"; }
  const char* GetDescriptiveName() override { return "OME TIFF"; }
  ///@}

protected:
  vtkOMETIFFReader();
  ~vtkOMETIFFReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  vtkOMETIFFReader(const vtkOMETIFFReader&) = delete;
  void operator=(const vtkOMETIFFReader&) = delete;

  class vtkOMEInternals;
  vtkOMEInternals* OMEInternals;
};

VTK_ABI_NAMESPACE_END
#endif
