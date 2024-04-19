// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageReader2
 * @brief   Superclass of binary file readers.
 *
 * vtkImageReader2 is a parent class for many VTK image readers.
 * It was written to simplify the interface of vtkImageReader.
 * It can also be used directly to read data without headers (raw).
 * It is a good super class for streaming readers that do not require
 * a mask or transform on the data. An example of reading a raw file is
 * shown below:
 * \code
 *  vtkSmartPointer<vtkImageReader2> reader =
 *   vtkSmartPointer<vtkImageReader2>::New();
 * reader->SetFilePrefix(argv[1]);
 * reader->SetDataExtent(0, 63, 0, 63, 1, 93);
 * reader->SetDataSpacing(3.2, 3.2, 1.5);
 * reader->SetDataOrigin(0.0, 0.0, 0.0);
 * reader->SetDataScalarTypeToUnsignedShort();
 * reader->SetDataByteOrderToLittleEndian();
 * reader->UpdateWholeExtent();
 * \endcode
 *
 * @sa
 * vtkJPEGReader vtkPNGReader vtkImageReader vtkGESignaReader
 */

#ifndef vtkImageReader2_h
#define vtkImageReader2_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTKIOIMAGE_EXPORT vtkImageReader2 : public vtkImageAlgorithm
{
public:
  static vtkImageReader2* New();
  vtkTypeMacro(vtkImageReader2, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name for the image file. If the data is stored in
   * multiple files, then use SetFileNames or SetFilePrefix instead.
   */
  virtual void SetFileName(VTK_FILEPATH const char*);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify a list of file names.  Each file must be a single slice,
   * and each slice must be of the same size. The files must be in the
   * correct order.
   * Use SetFileName when reading a volume (multiple slice), since
   * DataExtent will be modified after a SetFileNames call.
   */
  virtual void SetFileNames(vtkStringArray*);
  vtkGetObjectMacro(FileNames, vtkStringArray);
  ///@}

  ///@{
  /**
   * Specify file prefix for the image file or files.  This can be
   * used in place of SetFileName or SetFileNames if the filenames
   * follow a specific naming pattern, but you must explicitly set
   * the DataExtent so that the reader will know what range of slices
   * to load.
   */
  virtual void SetFilePrefix(VTK_FILEPATH const char*);
  vtkGetFilePathMacro(FilePrefix);
  ///@}

  ///@{
  /**
   * The snprintf-style format string used to build filename from
   * FilePrefix and slice number.
   */
  virtual void SetFilePattern(VTK_FILEPATH const char*);
  vtkGetFilePathMacro(FilePattern);
  ///@}

  /**
   * Specify the in memory image buffer.
   * May be used by a reader to allow the reading
   * of an image from memory instead of from file.
   */
  virtual void SetMemoryBuffer(const void*);
  virtual const void* GetMemoryBuffer() { return this->MemoryBuffer; }

  /**
   * Specify the in memory image buffer length.
   */
  virtual void SetMemoryBufferLength(vtkIdType buflen);
  vtkIdType GetMemoryBufferLength() { return this->MemoryBufferLength; }

  /**
   * Set the data type of pixels in the file.
   * If you want the output scalar type to have a different value, set it
   * after this method is called.
   */
  virtual void SetDataScalarType(int type);
  virtual void SetDataScalarTypeToFloat() { this->SetDataScalarType(VTK_FLOAT); }
  virtual void SetDataScalarTypeToDouble() { this->SetDataScalarType(VTK_DOUBLE); }
  virtual void SetDataScalarTypeToInt() { this->SetDataScalarType(VTK_INT); }
  virtual void SetDataScalarTypeToUnsignedInt() { this->SetDataScalarType(VTK_UNSIGNED_INT); }
  virtual void SetDataScalarTypeToShort() { this->SetDataScalarType(VTK_SHORT); }
  virtual void SetDataScalarTypeToUnsignedShort() { this->SetDataScalarType(VTK_UNSIGNED_SHORT); }
  virtual void SetDataScalarTypeToChar() { this->SetDataScalarType(VTK_CHAR); }
  virtual void SetDataScalarTypeToSignedChar() { this->SetDataScalarType(VTK_SIGNED_CHAR); }
  virtual void SetDataScalarTypeToUnsignedChar() { this->SetDataScalarType(VTK_UNSIGNED_CHAR); }

  ///@{
  /**
   * Get the file format.  Pixels are this type in the file.
   */
  vtkGetMacro(DataScalarType, int);
  ///@}

  ///@{
  /**
   * Set/Get the number of scalar components
   */
  vtkSetMacro(NumberOfScalarComponents, int);
  vtkGetMacro(NumberOfScalarComponents, int);
  ///@}

  ///@{
  /**
   * Get/Set the extent of the data on disk.
   */
  vtkSetVector6Macro(DataExtent, int);
  vtkGetVector6Macro(DataExtent, int);
  ///@}

  ///@{
  /**
   * The number of dimensions stored in a file. This defaults to two.
   */
  vtkSetMacro(FileDimensionality, int);
  int GetFileDimensionality() { return this->FileDimensionality; }
  ///@}

  ///@{
  /**
   * Set/Get the spacing of the data in the file.
   */
  vtkSetVector3Macro(DataSpacing, double);
  vtkGetVector3Macro(DataSpacing, double);
  ///@}

  ///@{
  /**
   * Set/Get the origin of the data (location of first pixel in the file).
   */
  vtkSetVector3Macro(DataOrigin, double);
  vtkGetVector3Macro(DataOrigin, double);
  ///@}

  ///@{
  /**
   * Set/Get the direction of the data (9 elements: 3x3 matrix).
   */
  vtkSetVectorMacro(DataDirection, double, 9);
  vtkGetVectorMacro(DataDirection, double, 9);
  ///@}

  ///@{
  /**
   * Get the size of the header computed by this object.
   */
  unsigned long GetHeaderSize();
  unsigned long GetHeaderSize(unsigned long slice);
  ///@}

  /**
   * If there is a tail on the file, you want to explicitly set the
   * header size.
   */
  virtual void SetHeaderSize(unsigned long size);

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
   * otherwise SetDataByteOrderToBigEndian.
   */
  virtual void SetDataByteOrderToBigEndian();
  virtual void SetDataByteOrderToLittleEndian();
  virtual int GetDataByteOrder();
  virtual void SetDataByteOrder(int);
  virtual const char* GetDataByteOrderAsString();
  ///@}

  ///@{
  /**
   * When reading files which start at an unusual index, this can be added
   * to the slice number when generating the file name (default = 0)
   */
  vtkSetMacro(FileNameSliceOffset, int);
  vtkGetMacro(FileNameSliceOffset, int);
  ///@}

  ///@{
  /**
   * When reading files which have regular, but non contiguous slices
   * (eg filename.1,filename.3,filename.5)
   * a spacing can be specified to skip missing files (default = 1)
   */
  vtkSetMacro(FileNameSliceSpacing, int);
  vtkGetMacro(FileNameSliceSpacing, int);
  ///@}

  ///@{
  /**
   * Set/Get the byte swapping to explicitly swap the bytes of a file.
   */
  vtkSetMacro(SwapBytes, vtkTypeBool);
  virtual vtkTypeBool GetSwapBytes() { return this->SwapBytes; }
  vtkBooleanMacro(SwapBytes, vtkTypeBool);
  ///@}

  istream* GetFile() { return this->File; }
  vtkGetVectorMacro(DataIncrements, unsigned long, 4);

  virtual int OpenFile();
  void CloseFile();
  virtual void SeekFile(int i, int j, int k);

  ///@{
  /**
   * Set/Get whether the data comes from the file starting in the lower left
   * corner or upper left corner.
   */
  vtkBooleanMacro(FileLowerLeft, vtkTypeBool);
  vtkGetMacro(FileLowerLeft, vtkTypeBool);
  vtkSetMacro(FileLowerLeft, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the internal file name
   */
  virtual void ComputeInternalFileName(int slice);
  vtkGetFilePathMacro(InternalFileName);
  ///@}

  /**
   * Return non zero if the reader can read the given file name.
   * Should be implemented by all sub-classes of vtkImageReader2.
   * For non zero return values the following values are to be used
   * 1 - I think I can read the file but I cannot prove it
   * 2 - I definitely can read the file
   * 3 - I can read the file and I have validated that I am the
   * correct reader for this file
   */
  virtual int CanReadFile(VTK_FILEPATH const char* vtkNotUsed(fname)) { return 0; }

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  virtual const char* GetFileExtensions() { return nullptr; }

  ///@{
  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  virtual const char* GetDescriptiveName() { return nullptr; }

protected:
  vtkImageReader2();
  ~vtkImageReader2() override;
  ///@}

  vtkStringArray* FileNames;

  char* InternalFileName;
  char* FileName;
  char* FilePrefix;
  char* FilePattern;
  int NumberOfScalarComponents;
  vtkTypeBool FileLowerLeft;

  const void* MemoryBuffer;
  vtkIdType MemoryBufferLength;

  istream* File;
  unsigned long DataIncrements[4];
  int DataExtent[6];
  vtkTypeBool SwapBytes;

  int FileDimensionality;
  unsigned long HeaderSize;
  int DataScalarType;
  unsigned long ManualHeaderSize;

  double DataSpacing[3];
  double DataOrigin[3];
  double DataDirection[9];

  int FileNameSliceOffset;
  int FileNameSliceSpacing;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  virtual void ExecuteInformation();
  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) override;
  virtual void ComputeDataIncrements();

private:
  vtkImageReader2(const vtkImageReader2&) = delete;
  void operator=(const vtkImageReader2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
