/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkProcessObject.h"

class vtkCellData;
class vtkDataArray;
class vtkDataCompressor;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkOutputStream;
class vtkPointData;
class vtkPoints;

class VTK_IO_EXPORT vtkXMLWriter : public vtkProcessObject
{
public:
  vtkTypeRevisionMacro(vtkXMLWriter,vtkProcessObject);
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
  
  // Description:
  // Get/Set the byte order of data written to the file.  The default
  // is the machine's hardware byte order.
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  
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
  // read when only part of the data are requested.
  vtkSetMacro(BlockSize, unsigned int);  
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
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension()=0;
  
  // Description:
  // Invoke the writer.  Returns 1 for success, 0 for failure.
  virtual int Write();
  
protected:
  vtkXMLWriter();
  ~vtkXMLWriter();
  
  // The name of the output file.
  char* FileName;
  
  // The output stream to which the XML is written.
  ostream* Stream;  
  
  // The output byte order.
  int ByteOrder;
  
  // The form of binary data to write.  Used by subclasses to choose
  // how to write data.
  int DataMode;
  
  // Whether to base64-encode the appended data section.
  int EncodeAppendedData;
  
  // The stream position at which appended data starts.
  unsigned long AppendedDataPosition;
  
  // Compression information.
  vtkDataCompressor* Compressor;
  unsigned int   BlockSize;  
  unsigned long  CompressionBlockNumber;
  unsigned int*  CompressionHeader;
  unsigned int   CompressionHeaderLength;
  unsigned long  CompressionHeaderPosition;
  
  // The output stream used to write binary and appended data.  May
  // transparently encode the data.
  vtkOutputStream* DataStream;
  
  // Allow subclasses to set the data stream.
  virtual void SetDataStream(vtkOutputStream*);
  vtkGetObjectMacro(DataStream, vtkOutputStream);  
  
  // Method defined by subclasses to write data.  Return 1 for
  // success, 0 for failure.
  virtual int WriteData()=0;
  
  // Method defined by subclasses to specify the data set's type name.
  virtual const char* GetDataSetName()=0;
  
  // Methods to define the file's major and minor version numbers.
  virtual int GetDataSetMajorVersion();
  virtual int GetDataSetMinorVersion();
  
  // Utility methods for subclasses.
  vtkDataSet* GetInputAsDataSet();  
  void StartFile();
  virtual void WriteFileAttributes();
  void EndFile();
  void StartAppendedData();
  void EndAppendedData();
  unsigned long ReserveAttributeSpace(const char* attr=0);
  unsigned long GetAppendedDataOffset();
  unsigned long WriteAppendedDataOffset(unsigned long streamPos,
                                        const char* attr=0);
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
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
  int WriteScalarAttribute(const char* name, vtkIdType data);
#endif
  
  int WriteVectorAttribute(const char* name, int length, int* data);
  int WriteVectorAttribute(const char* name, int length, float* data);
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
  int WriteVectorAttribute(const char* name, int length, vtkIdType* data);
#endif
  
  int WriteDataModeAttribute(const char* name);
  int WriteWordTypeAttribute(const char* name, int dataType);
  int WriteStringAttribute(const char* name, const char* value);
  
  unsigned long WriteDataArrayAppended(vtkDataArray* a, vtkIndent indent,
                                       const char* alternateName=0);
  void WriteDataArrayAppendedData(vtkDataArray* a, unsigned long pos);
  void WriteDataArrayInline(vtkDataArray* a, vtkIndent indent,
                            const char* alternateName=0);
  void WriteInlineData(void* data, int numWords, int wordType,
                       vtkIndent indent);
  
  // Methods for writing points, point data, and cell data.
  void WritePointDataInline(vtkPointData* pd, vtkIndent indent);
  void WriteCellDataInline(vtkCellData* cd, vtkIndent indent);
  unsigned long* WritePointDataAppended(vtkPointData* pd, vtkIndent indent);
  void WritePointDataAppendedData(vtkPointData* pd, unsigned long* pdPositions);
  unsigned long* WriteCellDataAppended(vtkCellData* cd, vtkIndent indent);
  void WriteCellDataAppendedData(vtkCellData* cd, unsigned long* cdPositions);
  void WriteAttributeIndices(vtkDataSetAttributes* dsa, char** names);
  unsigned long WritePointsAppended(vtkPoints* points, vtkIndent indent);
  void WritePointsAppendedData(vtkPoints* points, unsigned long pointsPosition);
  void WritePointsInline(vtkPoints* points, vtkIndent indent);
  void WriteCoordinatesInline(vtkDataArray* xc, vtkDataArray* yc,
                              vtkDataArray* zc, vtkIndent indent);
  unsigned long* WriteCoordinatesAppended(vtkDataArray* xc, vtkDataArray* yc,
                                          vtkDataArray* zc, vtkIndent indent);
  void WriteCoordinatesAppendedData(vtkDataArray* xc, vtkDataArray* yc,
                                    vtkDataArray* zc,
                                    unsigned long* cPositions);
  virtual vtkDataArray* CreateArrayForPoints(vtkDataArray* inArray);
  virtual vtkDataArray* CreateArrayForCells(vtkDataArray* inArray);
  virtual vtkDataArray* CreateExactCoordinates(vtkDataArray* inArray, int xyz);
  void WritePPointData(vtkPointData* pd, vtkIndent indent);
  void WritePCellData(vtkCellData* cd, vtkIndent indent);
  void WritePPoints(vtkPoints* points, vtkIndent indent);
  void WritePDataArray(vtkDataArray* a, vtkIndent indent,
                       const char* alternateName=0);
  
  // Internal utility methods.
  int WriteBinaryDataInternal(void* data, unsigned long size);
  void PerformByteSwap(void* data, int numWords, int wordSize);
  int CreateCompressionHeader(unsigned long size);
  int WriteCompressionBlock(unsigned char* data, unsigned long size);
  int WriteCompressionHeader();
  unsigned long GetWordTypeSize(int dataType);
  const char* GetWordTypeName(int dataType);
  
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);
  
private:
  vtkXMLWriter(const vtkXMLWriter&);  // Not implemented.
  void operator=(const vtkXMLWriter&);  // Not implemented.
};

#endif
