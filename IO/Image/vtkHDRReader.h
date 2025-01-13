// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDRReader
 * @brief   read Radiance HDR files
 *
 * vtkHDRReader is a source object that reads Radiance HDR files.
 * HDR files are converted into 32 bit images.
 */

#ifndef vtkHDRReader_h
#define vtkHDRReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"
#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkHDRReader : public vtkImageReader
{
public:
  static vtkHDRReader* New();
  vtkTypeMacro(vtkHDRReader, vtkImageReader);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum FormatType
  {
    FORMAT_32BIT_RLE_RGBE = 0,
    FORMAT_32BIT_RLE_XYZE
  };

  ///@{
  /**
   * Format is either 32-bit_rle_rgbe or 32-bit_rle_xyze.
   */
  vtkGetMacro(Format, int);
  ///@}

  ///@{
  /**
   * Get gamma correction.
   * Default value is 1.0.
   */
  vtkGetMacro(Gamma, double);
  ///@}

  ///@{
  /**
   * Get exposure.
   * Default value is 1.0.
   */
  vtkGetMacro(Exposure, double);
  ///@}

  ///@{
  /**
   * Get pixel aspect, the ratio of height by the width of a pixel.
   * Default value is 1.0.
   */
  vtkGetMacro(PixelAspect, double);
  ///@}

  /**
   * Is the given file a HDR file?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".hdr .pic"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Radiance HDR"; }

protected:
  vtkHDRReader();
  ~vtkHDRReader() override;

  std::string ProgramType;
  FormatType Format;
  double Gamma;
  double Exposure;
  double PixelAspect;

  /**
   * If true, the X axis has been flipped.
   */
  bool FlippedX = false;

  /**
   * If true, the Y axis is the X, and the height and width has been swapped.
   */
  bool SwappedAxis = false;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;
  bool HDRReaderUpdateSlice(float* outPtr, int* outExt);
  void HDRReaderUpdate(vtkImageData* data, float* outPtr);

  /**
   * If the stream has an error, close the file and return true.
   * Else return false.
   */
  bool HasError(istream* is);

  int GetWidth() const;
  int GetHeight() const;

  /**
   * Read the header data and fill attributes of HDRReader, as well as DataExtent.
   * Return true if the read succeed, else false.
   */
  bool ReadHeaderData();

  void ConvertAllDataFromRGBToXYZ(float* outPtr, int size);

  void FillOutPtrRLE(int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer);
  void FillOutPtrNoRLE(int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer);

  /**
   * Read the file from is into outPtr with no RLE encoding.
   * Return false if a reading error occurred, else true.
   */
  bool ReadAllFileNoRLE(istream* is, float* outPtr, int decrPtr, int* outExt);

  /**
   * Read a line of the file from is into lineBuffer with RLE encoding.
   * Return false if a reading error occurred, else true.
   */
  bool ReadLineRLE(istream* is, unsigned char* lineBufferPtr);

  /**
   * Standard conversion from rgbe to float pixels
   */
  void RGBE2Float(unsigned char rgbe[4], float& r, float& g, float& b);

  /**
   * Conversion from xyz to rgb float using the 3x3 convert matrix.
   * Inplace version, r,g,b are in xyz color space in input, in rgb color space
   * in output
   */
  static void XYZ2RGB(const float convertMatrix[3][3], double gamma, float& r, float& g, float& b);

private:
  vtkHDRReader(const vtkHDRReader&) = delete;
  void operator=(const vtkHDRReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
