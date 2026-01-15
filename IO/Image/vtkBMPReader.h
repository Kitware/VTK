// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBMPReader
 * @brief   read Windows BMP files
 *
 * vtkBMPReader is a source object that reads Windows BMP files.
 * This includes indexed and 24bit bitmaps
 * Usually, all BMPs are converted to 24bit RGB, but BMPs may be output
 * as 8bit images with a LookupTable if the Allow8BitBMP flag is set.
 *
 * BMPReader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * To read a volume, files must be of the form "FileName.<number>"
 * (e.g., foo.bmp.0, foo.bmp.1, ...). You must also specify the image
 * range. This range specifies the beginning and ending files to read (range
 * can be any pair of non-negative numbers).
 *
 * The default behavior is to read a single file. In this case, the form
 * of the file is simply "FileName" (e.g., foo.bmp).
 *
 * This reader supports reading streams.
 *
 * @sa
 * vtkBMPWriter
 */

#ifndef vtkBMPReader_h
#define vtkBMPReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"
VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

class VTKIOIMAGE_EXPORT vtkBMPReader : public vtkImageReader
{
public:
  static vtkBMPReader* New();
  vtkTypeMacro(vtkBMPReader, vtkImageReader);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns the depth of the BMP, either 8 or 24.
   */
  vtkGetMacro(Depth, int);
  ///@}

  ///@{
  /**
   * Return 1 if, after a quick check of file header, it looks like the provided file or stream
   * can be read as a BMP file. Return 0 if it is sure it cannot be read. The stream version may
   * move the stream cursor. This checks the magic "BM" and correct header InfoSize.
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;
  int CanReadFile(vtkResourceStream* stream) override;
  ///@}

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".bmp"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Windows BMP"; }

  ///@{
  /**
   * If this flag is set and the BMP reader encounters an 8bit file,
   * the data will be kept as unsigned chars and a lookuptable will be
   * exported
   */
  vtkSetMacro(Allow8BitBMP, vtkTypeBool);
  vtkGetMacro(Allow8BitBMP, vtkTypeBool);
  vtkBooleanMacro(Allow8BitBMP, vtkTypeBool);
  ///@}

  vtkGetObjectMacro(LookupTable, vtkLookupTable);

  ///@{
  /**
   * Returns the color lut.
   */
  vtkGetMacro(Colors, unsigned char*);
  ///@}

protected:
  vtkBMPReader();
  ~vtkBMPReader() override;

  unsigned char* Colors;
  short Depth;
  vtkTypeBool Allow8BitBMP;
  vtkLookupTable* LookupTable;

  void ComputeDataIncrements() override;
  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  vtkBMPReader(const vtkBMPReader&) = delete;
  void operator=(const vtkBMPReader&) = delete;

  static bool ReadAndCheckHeader(
    vtkResourceStream* stream, bool quiet, vtkTypeInt32& offset, vtkTypeInt32& infoSize);
};
VTK_ABI_NAMESPACE_END
#endif
