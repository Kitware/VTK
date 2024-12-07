// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLWriterBase
 * @brief Abstract base class for VTK-XML writers.
 *
 * vtkXMLWriterBase class was created to help refactor XML writers
 * (vtkXMLWriter and subclasses). Get/Set API on vtkXMLWriter is moved here while
 * all the gory implementation details are left in vtkXMLWriter. This enables use to create
 * a sibling hierarchy to vtkXMLWriter that uses a cleaner design to implement
 * the IO capabilities. Eventually, we vtkXMLWriter and its children will be
 * substituted by a parallel hierarchy at which point this class may merge with
 * it's new subclass.
 */

#ifndef vtkXMLWriterBase_h
#define vtkXMLWriterBase_h

#include "vtkAlgorithm.h"
#include "vtkIOXMLModule.h" // For export macro

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkDataCompressor;

class VTKIOXML_EXPORT vtkXMLWriterBase : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkXMLWriterBase, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Enumerate big and little endian byte order settings.
   */
  enum
  {
    BigEndian,
    LittleEndian
  };

  /**
   * Enumerate the supported data modes.
   * Ascii = Inline ascii data.
   * Binary = Inline binary data (base64 encoded, possibly compressed).
   * Appended = Appended binary data (possibly compressed and/or base64).
   */
  enum
  {
    Ascii,
    Binary,
    Appended
  };

  /**
   * Enumerate the supported vtkIdType bit lengths.
   * Int32 = File stores 32-bit values for vtkIdType.
   * Int64 = File stores 64-bit values for vtkIdType.
   */
  enum
  {
    Int32 = 32,
    Int64 = 64
  };

  /**
   * Enumerate the supported binary data header bit lengths.
   * UInt32 = File stores 32-bit binary data header elements.
   * UInt64 = File stores 64-bit binary data header elements.
   */
  enum
  {
    UInt32 = 32,
    UInt64 = 64
  };

  ///@{
  /**
   * Get/Set the byte order of data written to the file.  The default
   * is the machine's hardware byte order.
   */
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  void SetByteOrderToBigEndian() { this->SetByteOrder(BigEndian); }
  void SetByteOrderToLittleEndian() { this->SetByteOrder(LittleEndian); }
  ///@}

  ///@{
  /**
   * Get/Set the binary data header word type.  The default is UInt32.
   * Set to UInt64 when storing arrays requiring 64-bit indexing.
   */
  virtual void SetHeaderType(int);
  vtkGetMacro(HeaderType, int);
  void SetHeaderTypeToUInt32() { this->SetHeaderType(UInt32); }
  void SetHeaderTypeToUInt64() { this->SetHeaderType(UInt64); }
  ///@}

  ///@{
  /**
   * Get/Set the size of the vtkIdType values stored in the file.  The
   * default is the real size of vtkIdType.
   */
  virtual void SetIdType(int);
  vtkGetMacro(IdType, int);
  void SetIdTypeToInt32() { this->SetIdType(Int32); }
  void SetIdTypeToInt64() { this->SetIdType(Int64); }
  ///@}

  ///@{
  /**
   * Get/Set the name of the output file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  vtkSetMacro(WriteToOutputString, bool);
  vtkGetMacro(WriteToOutputString, bool);
  vtkBooleanMacro(WriteToOutputString, bool);
  std::string GetOutputString() { return this->OutputString; }
  ///@}

  ///@{
  /**
   * Get/Set the compressor used to compress binary and appended data
   * before writing to the file.  Default is a vtkZLibDataCompressor.
   */
  virtual void SetCompressor(vtkDataCompressor*);
  vtkGetObjectMacro(Compressor, vtkDataCompressor);
  ///@}

  enum CompressorType
  {
    NONE,
    ZLIB,
    LZ4,
    LZMA
  };

  ///@{
  /**
   * Convenience functions to set the compressor to certain known types.
   */
  void SetCompressorType(int compressorType);
  void SetCompressorTypeToNone() { this->SetCompressorType(NONE); }
  void SetCompressorTypeToLZ4() { this->SetCompressorType(LZ4); }
  void SetCompressorTypeToZLib() { this->SetCompressorType(ZLIB); }
  void SetCompressorTypeToLZMA() { this->SetCompressorType(LZMA); }
  ///@}

  ///@{
  /**
   * Get/Set compression level.
   * 1 (worst compression, fastest) ... 9 (best compression, slowest).
   */
  void SetCompressionLevel(int compressorLevel);
  vtkGetMacro(CompressionLevel, int);
  ///@}

  ///@{
  /**
   * Get/Set the block size used in compression.  When reading, this
   * controls the granularity of how much extra information must be
   * read when only part of the data are requested.  The value should
   * be a multiple of the largest scalar data type.
   */
  virtual void SetBlockSize(size_t blockSize);
  vtkGetMacro(BlockSize, size_t);
  ///@}

  ///@{
  /**
   * Get/Set the data mode used for the file's data.  The options are
   * vtkXMLWriter::Ascii, vtkXMLWriter::Binary, and
   * vtkXMLWriter::Appended.
   */
  vtkSetMacro(DataMode, int);
  vtkGetMacro(DataMode, int);
  void SetDataModeToAscii() { this->SetDataMode(Ascii); }
  void SetDataModeToBinary() { this->SetDataMode(Binary); }
  void SetDataModeToAppended() { this->SetDataMode(Appended); }
  ///@}

  ///@{
  /**
   * Get/Set whether the appended data section is base64 encoded.  If
   * encoded, reading and writing will be slower, but the file will be
   * fully valid XML and text-only.  If not encoded, the XML
   * specification will be violated, but reading and writing will be
   * fast.  The default is to do the encoding.
   */
  vtkSetMacro(EncodeAppendedData, bool);
  vtkGetMacro(EncodeAppendedData, bool);
  vtkBooleanMacro(EncodeAppendedData, bool);
  ///@}

  ///@{
  /**
   * Control whether to write "TimeValue" field data.
   * This TimeValue is the current time value in the pipeline information
   * key at the time of writing. Default to true.
   */
  vtkGetMacro(WriteTimeValue, bool);
  vtkSetMacro(WriteTimeValue, bool);
  vtkBooleanMacro(WriteTimeValue, bool);
  ///@}

  /**
   * Get the default file extension for files written by this writer.
   */
  virtual const char* GetDefaultFileExtension() = 0;

  /**
   * Invoke the writer.  Returns 1 for success, 0 for failure.
   */
  VTK_UNBLOCKTHREADS
  int Write();

protected:
  vtkXMLWriterBase();
  ~vtkXMLWriterBase() override;

  // Methods to define the file's major and minor version numbers.
  virtual int GetDataSetMajorVersion();
  virtual int GetDataSetMinorVersion();

  // The name of the output file.
  char* FileName;

  // Whether this object is writing to a string or a file.
  // Default is 0: write to file.
  bool WriteToOutputString;

  // The output string.
  std::string OutputString;

  // The output byte order.
  int ByteOrder;

  // The output binary header word type.
  int HeaderType;

  // The output vtkIdType.
  int IdType;

  // The form of binary data to write.  Used by subclasses to choose
  // how to write data.
  int DataMode;

  // Whether to base64-encode the appended data section.
  bool EncodeAppendedData;

  // Compression information.
  vtkDataCompressor* Compressor;
  size_t BlockSize;

  // Compression Level for vtkDataCompressor objects
  // 1 (worst compression, fastest) ... 9 (best compression, slowest)
  int CompressionLevel;

  // This variable is used to ease transition to new versions of VTK XML files.
  // If data that needs to be written satisfies certain conditions,
  // the writer can use the previous file version version.
  // For version change 0.1 -> 2.0 (UInt32 header) and 1.0 -> 2.0
  // (UInt64 header), if data does not have a vtkGhostType array,
  // the file is written with version: 0.1/1.0.
  bool UsePreviousVersion;

private:
  vtkXMLWriterBase(const vtkXMLWriterBase&) = delete;
  void operator=(const vtkXMLWriterBase&) = delete;

  bool WriteTimeValue = true;
};

VTK_ABI_NAMESPACE_END
#endif
