/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLWriter - Superclass for VTK's XML file writers.
// .SECTION Description
// vtkXMLWriter provides methods implementing most of the
// functionality needed to write VTK XML file formats.  Concrete
// subclasses provide actual writer implementations calling upon this
// functionality.

#ifndef vtkXMLWriter_h
#define vtkXMLWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkAlgorithm.h"
#include <vtksys/ios/sstream> // For ostringstream ivar

class vtkAbstractArray;
class vtkArrayIterator;
//BTX
template <class T> class vtkArrayIteratorTemplate;
//ETX
class vtkCellData;
class vtkDataArray;
class vtkDataCompressor;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkOutputStream;
class vtkPointData;
class vtkPoints;
class vtkFieldData;
class vtkXMLDataHeader;
//BTX
class vtkStdString;
class OffsetsManager;      // one per piece/per time
class OffsetsManagerGroup; // array of OffsetsManager
class OffsetsManagerArray; // array of OffsetsManagerGroup
//ETX

class VTKIOXML_EXPORT vtkXMLWriter : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkXMLWriter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Enumerate big and little endian byte order settings.
  enum { BigEndian, LittleEndian };
  //ETX

  //BTX
  // Description:
  // Enumerate the supported data modes.
  //   Ascii = Inline ascii data.
  //   Binary = Inline binary data (base64 encoded, possibly compressed).
  //   Appended = Appended binary data (possibly compressed and/or base64).
  enum { Ascii, Binary, Appended };
  //ETX

  //BTX
  // Description:
  // Enumerate the supported vtkIdType bit lengths.
  //   Int32 = File stores 32-bit values for vtkIdType.
  //   Int64 = File stores 64-bit values for vtkIdType.
  enum { Int32=32, Int64=64 };

  // Description:
  // Enumerate the supported binary data header bit lengths.
  //   UInt32 = File stores 32-bit binary data header elements.
  //   UInt64 = File stores 64-bit binary data header elements.
  enum { UInt32=32, UInt64=64 };
  //ETX

  // Description:
  // Get/Set the byte order of data written to the file.  The default
  // is the machine's hardware byte order.
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();

  // Description:
  // Get/Set the binary data header word type.  The default is UInt32.
  // Set to UInt64 when storing arrays requiring 64-bit indexing.
  virtual void SetHeaderType(int);
  vtkGetMacro(HeaderType, int);
  void SetHeaderTypeToUInt32();
  void SetHeaderTypeToUInt64();

  // Description:
  // Get/Set the size of the vtkIdType values stored in the file.  The
  // default is the real size of vtkIdType.
  virtual void SetIdType(int);
  vtkGetMacro(IdType, int);
  void SetIdTypeToInt32();
  void SetIdTypeToInt64();

  // Description:
  // Get/Set the name of the output file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Enable writing to an OutputString instead of the default, a file.
  vtkSetMacro(WriteToOutputString,int);
  vtkGetMacro(WriteToOutputString,int);
  vtkBooleanMacro(WriteToOutputString,int);
  std::string GetOutputString() { return this->OutputString; }

  // Description:
  // Get/Set the compressor used to compress binary and appended data
  // before writing to the file.  Default is a vtkZLibDataCompressor.
  virtual void SetCompressor(vtkDataCompressor*);
  vtkGetObjectMacro(Compressor, vtkDataCompressor);

//BTX
  enum CompressorType
    {
    NONE,
    ZLIB
    };
//ETX

  // Description:
  // Convenience functions to set the compressor to certain known types.
  void SetCompressorType(int compressorType);
  void SetCompressorTypeToNone()
    {
    this->SetCompressorType(NONE);
    }
  void SetCompressorTypeToZLib()
    {
    this->SetCompressorType(ZLIB);
    }

  // Description:
  // Get/Set the block size used in compression.  When reading, this
  // controls the granularity of how much extra information must be
  // read when only part of the data are requested.  The value should
  // be a multiple of the largest scalar data type.
  virtual void SetBlockSize(size_t blockSize);
  vtkGetMacro(BlockSize, size_t);

  // Description:
  // Get/Set the data mode used for the file's data.  The options are
  // vtkXMLWriter::Ascii, vtkXMLWriter::Binary, and
  // vtkXMLWriter::Appended.
  vtkSetMacro(DataMode, int);
  vtkGetMacro(DataMode, int);
  void SetDataModeToAscii();
  void SetDataModeToBinary();
  void SetDataModeToAppended();

  // Description:
  // Get/Set whether the appended data section is base64 encoded.  If
  // encoded, reading and writing will be slower, but the file will be
  // fully valid XML and text-only.  If not encoded, the XML
  // specification will be violated, but reading and writing will be
  // fast.  The default is to do the encoding.
  vtkSetMacro(EncodeAppendedData, int);
  vtkGetMacro(EncodeAppendedData, int);
  vtkBooleanMacro(EncodeAppendedData, int);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);
  vtkDataObject *GetInput(int port);
  vtkDataObject *GetInput() { return this->GetInput(0); };

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension()=0;

  // Description:
  // Invoke the writer.  Returns 1 for success, 0 for failure.
  int Write();

  // See the vtkAlgorithm for a description of what these do
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

  // Description:
  // Which TimeStep to write.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Which TimeStepRange to write.
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetVector2Macro(TimeStepRange, int);

  // Description:
  // Set the number of time steps
  vtkGetMacro(NumberOfTimeSteps,int);
  vtkSetMacro(NumberOfTimeSteps,int);

  // Description:
  // API to interface an outside the VTK pipeline control
  void Start();
  void Stop();
  void WriteNextTime(double time);

protected:
  vtkXMLWriter();
  ~vtkXMLWriter();

  virtual int RequestInformation(
                          vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // The name of the output file.
  char* FileName;

  // The output stream to which the XML is written.
  ostream* Stream;

  // Whether this object is writing to a string or a file.
  // Default is 0: write to file.
  int WriteToOutputString;

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
  int EncodeAppendedData;

  // The stream position at which appended data starts.
  vtkTypeInt64 AppendedDataPosition;

  // appended data offsets for field data
  OffsetsManagerGroup *FieldDataOM;  //one per array

  //BTX
  // We need a 32 bit signed integer type to which vtkIdType will be
  // converted if Int32 is specified for the IdType parameter to this
  // writer.
# if VTK_SIZEOF_SHORT == 4
  typedef short Int32IdType;
# elif VTK_SIZEOF_INT == 4
  typedef int Int32IdType;
# elif VTK_SIZEOF_LONG == 4
  typedef long Int32IdType;
# else
#  error "No native data type can represent a signed 32-bit integer."
# endif
  //ETX

  // Buffer for vtkIdType conversion.
  Int32IdType* Int32IdTypeBuffer;

  // The byte swapping buffer.
  unsigned char* ByteSwapBuffer;

  // Compression information.
  vtkDataCompressor* Compressor;
  size_t BlockSize;
  size_t CompressionBlockNumber;
  vtkXMLDataHeader* CompressionHeader;
  vtkTypeInt64 CompressionHeaderPosition;

  // The output stream used to write binary and appended data.  May
  // transparently encode the data.
  vtkOutputStream* DataStream;

  // Allow subclasses to set the data stream.
  virtual void SetDataStream(vtkOutputStream*);
  vtkGetObjectMacro(DataStream, vtkOutputStream);

  // Method to drive most of actual writing.
  virtual int WriteInternal();

  // Method defined by subclasses to write data.  Return 1 for
  // success, 0 for failure.
  virtual int WriteData() {return 1;};

  // Method defined by subclasses to specify the data set's type name.
  virtual const char* GetDataSetName()=0;

  // Methods to define the file's major and minor version numbers.
  virtual int GetDataSetMajorVersion();
  virtual int GetDataSetMinorVersion();

  // Utility methods for subclasses.
  vtkDataSet* GetInputAsDataSet();
  virtual int StartFile();
  virtual void WriteFileAttributes();
  virtual int EndFile();
  void DeleteAFile();
  void DeleteAFile(const char* name);

  virtual int WritePrimaryElement(ostream &os, vtkIndent indent);
  virtual void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  void StartAppendedData();
  void EndAppendedData();

  // Write enough space to go back and write the given attribute with
  // at most "length" characters in the value.  Returns the stream
  // position at which attribute should be later written.  The default
  // length of 20 is enough for a 64-bit integer written in decimal or
  // a double-precision floating point value written to 13 digits of
  // precision (the other 7 come from a minus sign, decimal place, and
  // a big exponent like "e+300").
  vtkTypeInt64 ReserveAttributeSpace(const char* attr, size_t length=20);

  vtkTypeInt64 GetAppendedDataOffset();
  void WriteAppendedDataOffset(vtkTypeInt64 streamPos,
                               vtkTypeInt64 &lastoffset,
                               const char* attr=0);
  void ForwardAppendedDataOffset(vtkTypeInt64 streamPos,
                                 vtkTypeInt64 offset,
                                 const char* attr=0);
  void ForwardAppendedDataDouble(vtkTypeInt64 streamPos,
                                 double value,
                                 const char* attr);

  int WriteScalarAttribute(const char* name, int data);
  int WriteScalarAttribute(const char* name, float data);
  int WriteScalarAttribute(const char* name, double data);
#ifdef VTK_USE_64BIT_IDS
  int WriteScalarAttribute(const char* name, vtkIdType data);
#endif

  int WriteVectorAttribute(const char* name, int length, int* data);
  int WriteVectorAttribute(const char* name, int length, float* data);
  int WriteVectorAttribute(const char* name, int length, double* data);
#ifdef VTK_USE_64BIT_IDS
  int WriteVectorAttribute(const char* name, int length, vtkIdType* data);
#endif

  int WriteDataModeAttribute(const char* name);
  int WriteWordTypeAttribute(const char* name, int dataType);
  int WriteStringAttribute(const char* name, const char* value);

  void WriteArrayHeader(vtkAbstractArray* a, vtkIndent indent,
    const char* alternateName, int writeNumTuples, int timestep);
  void WriteArrayFooter(ostream &os, vtkIndent indent, vtkAbstractArray *a, int shortFormat);
  void WriteArrayInline(vtkAbstractArray* a, vtkIndent indent,
    const char* alternateName=0, int writeNumTuples=0);
  void WriteInlineData(vtkAbstractArray* a, vtkIndent indent);

  void WriteArrayAppended(vtkAbstractArray* a, vtkIndent indent,
    OffsetsManager &offs, const char* alternateName=0,  int writeNumTuples=0,
    int timestep=0);
  int WriteAsciiData(vtkAbstractArray* a, vtkIndent indent);
  int WriteBinaryData(vtkAbstractArray* a);
  int WriteBinaryDataInternal(vtkAbstractArray* a);
  void WriteArrayAppendedData(vtkAbstractArray* a, vtkTypeInt64 pos,
                              vtkTypeInt64 &lastoffset);

  // Methods for writing points, point data, and cell data.
  void WriteFieldData(vtkIndent indent);
  void WriteFieldDataInline(vtkFieldData* fd, vtkIndent indent);
  void WritePointDataInline(vtkPointData* pd, vtkIndent indent);
  void WriteCellDataInline(vtkCellData* cd, vtkIndent indent);
  void WriteFieldDataAppended(vtkFieldData* fd, vtkIndent indent,
                              OffsetsManagerGroup *fdManager);
  void WriteFieldDataAppendedData(vtkFieldData* fd, int timestep,
                                  OffsetsManagerGroup *fdManager);
  void  WritePointDataAppended(vtkPointData* pd, vtkIndent indent,
                               OffsetsManagerGroup *pdManager);
  void WritePointDataAppendedData(vtkPointData* pd, int timestep,
                                  OffsetsManagerGroup *pdManager);
  void WriteCellDataAppended(vtkCellData* cd, vtkIndent indent,
                             OffsetsManagerGroup *cdManager);
  void WriteCellDataAppendedData(vtkCellData* cd, int timestep,
                                 OffsetsManagerGroup *cdManager);
  void WriteAttributeIndices(vtkDataSetAttributes* dsa, char** names);
  void WritePointsAppended(vtkPoints* points, vtkIndent indent, OffsetsManager *manager);
  void WritePointsAppendedData(vtkPoints* points, int timestep, OffsetsManager *pdManager);
  void WritePointsInline(vtkPoints* points, vtkIndent indent);
  void WriteCoordinatesInline(vtkDataArray* xc, vtkDataArray* yc,
                              vtkDataArray* zc, vtkIndent indent);
  void WriteCoordinatesAppended(vtkDataArray* xc, vtkDataArray* yc,
                                vtkDataArray* zc, vtkIndent indent,
                                OffsetsManagerGroup *coordManager);
  void WriteCoordinatesAppendedData(vtkDataArray* xc, vtkDataArray* yc,
                                    vtkDataArray* zc, int timestep,
                                    OffsetsManagerGroup *coordManager);
  void WritePPointData(vtkPointData* pd, vtkIndent indent);
  void WritePCellData(vtkCellData* cd, vtkIndent indent);
  void WritePPoints(vtkPoints* points, vtkIndent indent);
  void WritePArray(vtkAbstractArray* a, vtkIndent indent,
    const char* alternateName=0);
  void WritePCoordinates(vtkDataArray* xc, vtkDataArray* yc,
                         vtkDataArray* zc, vtkIndent indent);

  // Internal utility methods.
  int WriteBinaryDataBlock(unsigned char* in_data, size_t numWords, int wordType);
  void PerformByteSwap(void* data, size_t numWords, size_t wordSize);
  int CreateCompressionHeader(size_t size);
  int WriteCompressionBlock(unsigned char* data, size_t size);
  int WriteCompressionHeader();
  size_t GetWordTypeSize(int dataType);
  const char* GetWordTypeName(int dataType);
  size_t GetOutputWordTypeSize(int dataType);

  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);

  // The current range over which progress is moving.  This allows for
  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float range[2]);
  virtual void SetProgressRange(const float range[2], int curStep, int numSteps);
  virtual void SetProgressRange(const float range[2], int curStep, const float* fractions);
  virtual void SetProgressPartial(float fraction);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

  ofstream* OutFile;
  vtksys_ios::ostringstream* OutStringStream;

  int OpenStream();
  int OpenFile();
  int OpenString();
  void CloseStream();
  void CloseFile();
  void CloseString();

  // The timestep currently being written
  int TimeStep;
  int CurrentTimeIndex;
  int NumberOfTimeSteps;
  // Store the range of time steps
  int TimeStepRange[2];

  // Dummy boolean var to start/stop the continue executing:
  // when using the Start/Stop/WriteNextTime API
  int UserContinueExecuting; //can only be -1 = invalid, 0 = stop, 1 = start

  // This variable is used to ease transition to new versions of VTK XML files.
  // If data that needs to be written satisfies certain conditions,
  // the writer can use the previous file version version.
  // For version change 0.1 -> 2.0 (UInt32 header) and 1.0 -> 2.0
  // (UInt64 header), if data does not have a vtkGhostType array,
  // the file is written with version: 0.1/1.0.
  bool UsePreviousVersion;

  vtkTypeInt64 *NumberOfTimeValues; //one per piece / per timestep
  //BTX
  friend class vtkXMLWriterHelper;
  //ETX

private:
  vtkXMLWriter(const vtkXMLWriter&);  // Not implemented.
  void operator=(const vtkXMLWriter&);  // Not implemented.
};

#endif
