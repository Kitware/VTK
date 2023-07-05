// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) GeometryFactory
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSEPReader
 * @brief Stanford Exploration Project files reader.
 *
 * This reader takes a .H file that points to a .H@ file and contains
 * all the information to interpret the raw data in the  .H@ file.
 */

#ifndef vtkSEPReader_h
#define vtkSEPReader_h

#include "vtkIOImageModule.h" // For export macro

#include "vtkExtentTranslator.h" // for vtkExtentTranslator
#include "vtkImageAlgorithm.h"
#include "vtkNew.h" // for ivars

#include <array>   // for std::array
#include <cstdint> // for std::uint8_t and std::uint32_t
#include <string>  // for std::string

namespace details
{
VTK_ABI_NAMESPACE_BEGIN
enum class EndiannessType : std::uint8_t
{
  SEP_LITTLE_ENDIAN = 0,
  SEP_BIG_ENDIAN = 1
};

static constexpr int SEP_READER_MAX_DIMENSION = 32u;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;

class VTKIOIMAGE_EXPORT vtkSEPReader : public vtkImageAlgorithm
{
public:
  static vtkSEPReader* New();
  vtkTypeMacro(vtkSEPReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Specify file name for the SEP Header file.
   */
  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);

  /**
   * @brief When 2D mode is true, the third dimension is
   * ignored and the output is in 2D.
   **/
  vtkGetMacro(OutputGridDimension, int);
  vtkSetMacro(OutputGridDimension, int);

  /**
   * @brief Specify extent translator split mode.
   * Default: vtkExtentTranslator::BLOCK_MODE
   **/
  vtkSetMacro(ExtentSplitMode, int);
  vtkGetMacro(ExtentSplitMode, int);

  vtkGetMacro(DataOrigin, VTK_FUTURE_CONST double*);
  vtkGetMacro(DataSpacing, VTK_FUTURE_CONST double*);

  /**
   * Array containing the name of all dimensions.
   * Contains ESize elements.
   **/
  vtkGetObjectMacro(AllDimensions, vtkStringArray);

  /**
   * Array containing the name and the size of all dimensions.
   * The two first entries are the header. Contains 2*ESize elements.
   **/
  vtkGetObjectMacro(AllRanges, vtkStringArray);

  /**
   * Specify the name for each spatial / fixed dimension.
   * ZDimension is only used for 3D output.
   * FixedDimension2 is only used for 2D output.
   **/
  vtkSetStdStringFromCharMacro(XDimension);
  vtkSetStdStringFromCharMacro(YDimension);
  vtkSetStdStringFromCharMacro(ZDimension);
  vtkSetStdStringFromCharMacro(FixedDimension1);
  vtkSetStdStringFromCharMacro(FixedDimension2);

  vtkSetMacro(FixedDimensionValue1, int);
  vtkSetMacro(FixedDimensionValue2, int);
  vtkGetVector2Macro(FixedDimRange, int);

  bool CanReadFile(VTK_FILEPATH const char*);

  std::array<std::int32_t, 6> ComputeExtent() const;

protected:
  vtkSEPReader();

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation* request, vtkInformationVector**, vtkInformationVector*) override;

  bool ReadHeader();
  bool ReadData(vtkImageData*, int*);

  /**
   * Exposed Properties
   */
  std::string FileName;
  int OutputGridDimension = 3;
  int ExtentSplitMode = vtkExtentTranslator::BLOCK_MODE;
  double DataOrigin[details::SEP_READER_MAX_DIMENSION];
  double DataSpacing[details::SEP_READER_MAX_DIMENSION];
  std::string XDimension = "CDP";
  std::string YDimension = "LINE";
  std::string ZDimension = "DEPTH"; // used only in 3D
  std::string FixedDimension1 = "OFFSET";
  std::string FixedDimension2 = "DEPTH"; // used only in 2D
  int FixedDimensionValue1 = details::SEP_READER_MAX_DIMENSION;
  int FixedDimensionValue2 = details::SEP_READER_MAX_DIMENSION;
  int FixedDimRange[2] = { 0, 0 };

  vtkNew<vtkStringArray> AllDimensions;
  vtkNew<vtkStringArray> AllRanges;

private:
  enum class DataFormatType : std::uint8_t
  {
    XDR_FLOAT = 0,
    XDR_INT = 1,
    XDR_DOUBLE = 2
  };

  /**
   * Internal Variables
   */
  DataFormatType DataFormat = DataFormatType::XDR_FLOAT;
  details::EndiannessType Endianness;
  int Dimensions[details::SEP_READER_MAX_DIMENSION];
  double OutputSpacing[3];
  double OutputOrigin[3];
  std::string Label[details::SEP_READER_MAX_DIMENSION];
  std::string DataFileType;
  std::string BinaryFilename;
  int ESize = 4;
  int XArrayId = details::SEP_READER_MAX_DIMENSION;
  int YArrayId = details::SEP_READER_MAX_DIMENSION;
  int ZArrayId = details::SEP_READER_MAX_DIMENSION;
  int FixedDimension1ArrayId = details::SEP_READER_MAX_DIMENSION;
  int FixedDimension2ArrayId = details::SEP_READER_MAX_DIMENSION;

  void ReadDataPiece(FILE* file, char*& dataOutput, vtkIdType offset, vtkIdType range);

  vtkSEPReader(const vtkSEPReader&) = delete;
  void operator=(const vtkSEPReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSEPReader_h
