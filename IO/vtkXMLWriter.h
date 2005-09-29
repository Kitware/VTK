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

#ifndef __vtkXMLWriter_h
#define __vtkXMLWriter_h

#include "vtkAlgorithm.h"

class vtkCellData;
class vtkDataArray;
class vtkDataCompressor;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkOutputStream;
class vtkPointData;
class vtkPoints;
class vtkFieldData;
//BTX
class OffsetsManager;      // one per piece/per time
class OffsetsManagerGroup; // array of OffsetsManager
class OffsetsManagerArray; // array of OffsetsManagerGroup
//ETX

class VTK_IO_EXPORT vtkXMLWriter : public vtkAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkXMLWriter,vtkAlgorithm);
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
  //ETX
  
  // Description:
  // Get/Set the byte order of data written to the file.  The default
  // is the machine's hardware byte order.
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  
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
  // Get/Set the compressor used to compress binary and appended data
  // before writing to the file.  Default is no compression.
  virtual void SetCompressor(vtkDataCompressor*);
  vtkGetObjectMacro(Compressor, vtkDataCompressor);
  
  // Description:
  // Get/Set the block size used in compression.  When reading, this
  // controls the granularity of how much extra information must be
  // read when only part of the data are requested.  The value should
  // be a multiple of the largest scalar data type.
  virtual void SetBlockSize(unsigned int blockSize);
  vtkGetMacro(BlockSize, unsigned int);
  
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
  // Set/Get an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline
  void SetInput(vtkDataObject *);
  void SetInput(int, vtkDataObject*);
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
  
  // The output byte order.
  int ByteOrder;
  
  // The output vtkIdType.
  int IdType;
  
  // The form of binary data to write.  Used by subclasses to choose
  // how to write data.
  int DataMode;
  
  // Whether to base64-encode the appended data section.
  int EncodeAppendedData;
  
  // The stream position at which appended data starts.
  unsigned long AppendedDataPosition;

  // appended data offsets for field data
  OffsetsManagerGroup *FieldDataOM;  //one per array

  //BTX
  // We need a 32 bit unsigned integer type for platform-independent
  // binary headers.  Note that this is duplicated in
  // vtkXMLDataParser.h.
#if VTK_SIZEOF_SHORT == 4
  typedef unsigned short HeaderType;
#elif VTK_SIZEOF_INT == 4
  typedef unsigned int HeaderType;
#elif VTK_SIZEOF_LONG == 4
  typedef unsigned long HeaderType;
#else
# error "No native data type can represent an unsigned 32-bit integer."
#endif
  //ETX

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
  unsigned int   BlockSize;  
  unsigned long  CompressionBlockNumber;
  HeaderType*    CompressionHeader;
  unsigned int   CompressionHeaderLength;
  unsigned long  CompressionHeaderPosition;
  
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
  int StartFile();
  virtual void WriteFileAttributes();
  int EndFile();
  void DeleteAFile();
  void DeleteAFile(const char* name);

  virtual int WritePrimaryElement(ostream &os, vtkIndent indent);
  virtual void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  void StartAppendedData();
  void EndAppendedData();
  unsigned long ReserveAttributeSpace(const char* attr=0);
  unsigned long GetAppendedDataOffset();
  unsigned long WriteAppendedDataOffset(unsigned long streamPos,
                                        unsigned long &lastoffset,
                                        const char* attr=0);
  unsigned long ForwardAppendedDataOffset(unsigned long streamPos,
                                         unsigned long offset,
                                         const char* attr=0);
  unsigned long ForwardAppendedDataDouble(unsigned long streamPos,
                                          double value,
                                          const char* attr);
  int WriteBinaryData(void* data, int numWords, int wordType);
  
  int WriteBinaryData(char* data)
    { return this->WriteBinaryData(data, static_cast<int>(strlen(data)),
                                   VTK_CHAR); }
  int WriteBinaryData(char* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_CHAR); }
  int WriteBinaryData(unsigned char* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_UNSIGNED_CHAR); }
  int WriteBinaryData(short* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_SHORT); }
  int WriteBinaryData(unsigned short* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_UNSIGNED_SHORT); }
  int WriteBinaryData(int* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_INT); }
  int WriteBinaryData(unsigned int* data, int numWords)
    { return this->WriteBinaryData(data, numWords, VTK_UNSIGNED_INT); }
  
  int WriteAsciiData(void* data, int numWords, int wordType, vtkIndent indent);
  
  int WriteAsciiData(char* data, vtkIndent indent)
    { return this->WriteAsciiData(data, static_cast<int>(strlen(data)),
                                  VTK_CHAR, indent); }
  int WriteAsciiData(char* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_CHAR, indent); }
  int WriteAsciiData(unsigned char* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_UNSIGNED_CHAR, indent); }
  int WriteAsciiData(short* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_SHORT, indent); }
  int WriteAsciiData(unsigned short* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_UNSIGNED_SHORT, indent); }
  int WriteAsciiData(int* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_INT, indent); }
  int WriteAsciiData(unsigned int* data, int numWords, vtkIndent indent)
    { return this->WriteAsciiData(data, numWords, VTK_UNSIGNED_INT, indent); }
  
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
  
  void WriteDataArrayAppended(vtkDataArray* a, vtkIndent indent,
                              OffsetsManager &offs,
                              const char* alternateName=0, 
                              int writeNumTuples=0, int timestep=0);
  void WriteDataArrayAppendedData(vtkDataArray* a, unsigned long pos, unsigned long &lastoffset);
  void WriteDataArrayInline(vtkDataArray* a, vtkIndent indent,
                            const char* alternateName=0, int writeNumTuples=0);
  void WriteInlineData(void* data, int numWords, int wordType,
                       vtkIndent indent);
  
  void WriteDataArrayHeader(vtkDataArray* a, vtkIndent indent,
                            const char* alternateName, 
                            int writeNumTuples, int timestep);
  void WriteDataArrayFooter(ostream &os, vtkIndent indent);
  
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
  virtual vtkDataArray* CreateArrayForPoints(vtkDataArray* inArray);
  virtual vtkDataArray* CreateArrayForCells(vtkDataArray* inArray);
  virtual vtkDataArray* CreateExactCoordinates(vtkDataArray* inArray, int xyz);
  void WritePPointData(vtkPointData* pd, vtkIndent indent);
  void WritePCellData(vtkCellData* cd, vtkIndent indent);
  void WritePPoints(vtkPoints* points, vtkIndent indent);
  void WritePDataArray(vtkDataArray* a, vtkIndent indent,
                       const char* alternateName=0);
  void WritePCoordinates(vtkDataArray* xc, vtkDataArray* yc,
                         vtkDataArray* zc, vtkIndent indent);
  
  // Internal utility methods.
  int WriteBinaryDataInternal(void* data, int numWords, int wordType);
  int WriteBinaryDataBlock(unsigned char* in_data, int numWords, int wordType);
  void PerformByteSwap(void* data, int numWords, int wordSize);
  int CreateCompressionHeader(unsigned long size);
  int WriteCompressionBlock(unsigned char* data, unsigned long size);
  int WriteCompressionHeader();
  unsigned long GetWordTypeSize(int dataType);
  const char* GetWordTypeName(int dataType);
  unsigned long GetOutputWordTypeSize(int dataType);
  
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);
  
  // The current range over which progress is moving.  This allows for
  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float* range);
  virtual void SetProgressRange(float* range, int curStep, int numSteps);
  virtual void SetProgressRange(float* range, int curStep, float* fractions);
  virtual void SetProgressPartial(float fraction);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

  ofstream* OutFile;

  int OpenFile();
  void CloseFile();

  // The timestep currently being written
  int TimeStep;
  int CurrentTimeIndex;
  int NumberOfTimeSteps;
  // Store the range of time steps
  int TimeStepRange[2];

  // Dummy boolean var to start/stop the continue executing:
  // when using the Start/Stop/WriteNextTime API
  int UserContinueExecuting; //can only be -1 = invalid, 0 = stop, 1 = start

  unsigned long *NumberOfTimeValues; //one per piece / per timestep

private:
  vtkXMLWriter(const vtkXMLWriter&);  // Not implemented.
  void operator=(const vtkXMLWriter&);  // Not implemented.
};

#endif
