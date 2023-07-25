// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticleReader
 * @brief   Read ASCII or binary particle
 *                            data and (optionally) one scalar
 *                            value associated with each particle.
 *
 * vtkParticleReader reads either a binary or a text file of
 *  particles. Each particle can have associated with it an optional
 *  scalar value. So the format is: x, y, z, scalar
 *  (all floats or doubles). The text file can consist of a comma
 *  delimited set of values. In most cases vtkParticleReader can
 *  automatically determine whether the file is text or binary.
 *  The data can be either float or double.
 *  Progress updates are provided.
 *  With respect to binary files, random access into the file to read
 *  pieces is supported.
 *
 */

#ifndef vtkParticleReader_h
#define vtkParticleReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

VTK_ABI_NAMESPACE_BEGIN
class VTKIOGEOMETRY_EXPORT vtkParticleReader : public vtkPolyDataAlgorithm
{
public:
  static vtkParticleReader* New();
  vtkTypeMacro(vtkParticleReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian
   * otherwise SetDataByteOrderToBigEndian. Not used when reading
   * text files.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char* GetDataByteOrderAsString();
  ///@}

  ///@{
  /**
   * Set/Get the byte swapping to explicitly swap the bytes of a file.
   * Not used when reading text files.
   */
  vtkSetMacro(SwapBytes, vtkTypeBool);
  vtkTypeBool GetSwapBytes() { return this->SwapBytes; }
  vtkBooleanMacro(SwapBytes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Default: 1. If 1 then each particle has a value associated with it.
   */
  vtkSetMacro(HasScalar, vtkTypeBool);
  vtkGetMacro(HasScalar, vtkTypeBool);
  vtkBooleanMacro(HasScalar, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get/Set the file type.  The options are:
   * - FILE_TYPE_IS_UNKNOWN (default) the class
   * will attempt to determine the file type.
   * If this fails then you should set the file type
   * yourself.
   * - FILE_TYPE_IS_TEXT the file type is text.
   * - FILE_TYPE_IS_BINARY the file type is binary.
   */
  vtkSetClampMacro(FileType, int, FILE_TYPE_IS_UNKNOWN, FILE_TYPE_IS_BINARY);
  vtkGetMacro(FileType, int);
  void SetFileTypeToUnknown() { this->SetFileType(FILE_TYPE_IS_UNKNOWN); }
  void SetFileTypeToText() { this->SetFileType(FILE_TYPE_IS_TEXT); }
  void SetFileTypeToBinary() { this->SetFileType(FILE_TYPE_IS_BINARY); }
  ///@}

  ///@{
  /**
   * Get/Set the data type.  The options are:
   * - VTK_FLOAT (default) single precision floating point.
   * - VTK_DOUBLE double precision floating point.
   */
  vtkSetClampMacro(DataType, int, VTK_FLOAT, VTK_DOUBLE);
  vtkGetMacro(DataType, int);
  void SetDataTypeToFloat() { this->SetDataType(VTK_FLOAT); }
  void SetDataTypeToDouble() { this->SetDataType(VTK_DOUBLE); }
  ///@}

protected:
  vtkParticleReader();
  ~vtkParticleReader() override;

  void OpenFile();

  char* FileName;
  istream* File;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@{
  /**
   * The format that will be read if the file is a text file is:
   * x, y, z, s (where s is some scalar value associated with the particle).
   * Each line corresponding to a particle is terminated with a line feed.
   * If y, z, or s is missing, zero is substituted for them.
   * Comment lines in the file are handled as follows:
   * 1) Any line containing "\/\/" "\#" "\%" anywhere in the line is discarded.
   * 2) Lines containing "\/\*" are discarded until a "\*\/" is found. The line
   * following the "\*\/" will be read.
   */
  int ProduceOutputFromTextFileDouble(vtkInformationVector* outputVector);
  int ProduceOutputFromTextFileFloat(vtkInformationVector* outputVector);
  ///@}

  ///@{
  /**
   * This reader assumes that the file is binary and consists of floating
   * point values by default.
   */
  int ProduceOutputFromBinaryFileDouble(vtkInformationVector* outputVector);
  int ProduceOutputFromBinaryFileFloat(vtkInformationVector* outputVector);
  ///@}

  /**
   * Determine the type of file based on an analysis of its contents.
   * Up to 5000 bytes of the file are read and classified. The classification
   * of a file as either binary or text is based on the proportions of bytes in
   * various classifications. The classification of the file is not infallible
   * but should work correctly most of the time. If it fails, use SetFileTypeToText()
   * or SetFileTypeToBinary() to set the file type.
   * This algorithm probably only identifies ASCII text correctly and will not
   * work for UTF-8 UCS-2 (or UTF-16) or UCS-4 or EBCIDIC.
   */
  int DetermineFileType();

  /**
   * Update of the progress.
   */
  void DoProgressUpdate(size_t& bytesRead, size_t& fileLength);

  /**
   * Enumerate the supported file types.
   *
   * - FILE_TYPE_IS_UNKNOWN, (default) the class will attempt to determine the file type.
   * - FILE_TYPE_IS_TEXT, the file type is text.
   * - FILE_TYPE_IS_BINARY, the file type is binary.
   */
  enum FILE_TYPE
  {
    FILE_TYPE_IS_UNKNOWN = 0,
    FILE_TYPE_IS_TEXT,
    FILE_TYPE_IS_BINARY
  };

  vtkTypeBool HasScalar;
  /**
   * Used to decide which reader should be used.
   */
  int FileType;
  /**
   * Used to specify the data type.
   */
  int DataType;

  /**
   * Set an alliquot of bytes.
   */
  size_t Alliquot;
  /**
   * Count of the number of alliquots processed.
   */
  size_t Count;

  vtkTypeBool SwapBytes;
  size_t NumberOfPoints;

private:
  vtkParticleReader(const vtkParticleReader&) = delete;
  void operator=(const vtkParticleReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
