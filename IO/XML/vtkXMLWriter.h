// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLWriter
 * @brief   Superclass for VTK's XML file writers.
 *
 * vtkXMLWriter provides methods implementing most of the
 * functionality needed to write VTK XML file formats.  Concrete
 * subclasses provide actual writer implementations calling upon this
 * functionality.
 *
 * @par Thanks
 * CompressionLevel getters/setters exposed by Quincy Wofford
 * (qwofford@lanl.gov) and John Patchett (patchett@lanl.gov),
 * Los Alamos National Laboratory (2017)
 */

#ifndef vtkXMLWriter_h
#define vtkXMLWriter_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriterBase.h"

#include <sstream> // For ostringstream ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkArrayIterator;

template <class T>
class vtkArrayIteratorTemplate;

class vtkCellData;
class vtkDataArray;
class vtkDataCompressor;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkOutputStream;
class vtkPointData;
class vtkPoints;
class vtkFieldData;
class vtkXMLDataHeader;

class vtkStdString;
class OffsetsManager;      // one per piece/per time
class OffsetsManagerGroup; // array of OffsetsManager
class OffsetsManagerArray; // array of OffsetsManagerGroup

class VTKIOXML_EXPORT vtkXMLWriter : public vtkXMLWriterBase
{
public:
  vtkTypeMacro(vtkXMLWriter, vtkXMLWriterBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  vtkDataObject* GetInput(int port);
  vtkDataObject* GetInput() { return this->GetInput(0); }
  ///@}

  // See the vtkAlgorithm for a description of what these do
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  /**
   * Set the number of time steps
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  vtkSetMacro(NumberOfTimeSteps, int);
  ///@}

  ///@{
  /**
   * API to interface an outside the VTK pipeline control
   */
  void Start();
  void Stop();
  void WriteNextTime(double time);
  ///@}

protected:
  vtkXMLWriter();
  ~vtkXMLWriter() override;

  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // The output stream to which the XML is written.
  ostream* Stream;

  // The stream position at which appended data starts.
  vtkTypeInt64 AppendedDataPosition;

  // appended data offsets for field data
  OffsetsManagerGroup* FieldDataOM; // one per array

  // We need a 32 bit signed integer type to which vtkIdType will be
  // converted if Int32 is specified for the IdType parameter to this
  // writer.
#if VTK_SIZEOF_SHORT == 4
  typedef short Int32IdType;
#elif VTK_SIZEOF_INT == 4
  typedef int Int32IdType;
#elif VTK_SIZEOF_LONG == 4
  typedef long Int32IdType;
#else
#error "No native data type can represent a signed 32-bit integer."
#endif

  // Buffer for vtkIdType conversion.
  Int32IdType* Int32IdTypeBuffer;

  // The byte swapping buffer.
  unsigned char* ByteSwapBuffer;

  // Compression information.
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
  virtual int WriteData() { return 1; }

  // Method defined by subclasses to specify the data set's type name.
  virtual const char* GetDataSetName() = 0;

  // Utility methods for subclasses.
  vtkDataSet* GetDataSetInput();
  VTK_DEPRECATED_IN_9_5_0("Use GetDataSetInput() instead.")
  vtkDataSet* GetInputAsDataSet() { return this->GetDataSetInput(); }
  virtual int StartFile();
  virtual void WriteFileAttributes();
  virtual int EndFile();
  void DeleteAFile();
  void DeleteAFile(const char* name);

  virtual int WritePrimaryElement(ostream& os, vtkIndent indent);
  virtual void WritePrimaryElementAttributes(ostream& os, vtkIndent indent);
  void StartAppendedData();
  void EndAppendedData();

  // Write enough space to go back and write the given attribute with
  // at most "length" characters in the value.  Returns the stream
  // position at which attribute should be later written.  The default
  // length of 20 is enough for a 64-bit integer written in decimal or
  // a double-precision floating point value written to 13 digits of
  // precision (the other 7 come from a minus sign, decimal place, and
  // a big exponent like "e+300").
  vtkTypeInt64 ReserveAttributeSpace(const char* attr, size_t length = 20);

  vtkTypeInt64 GetAppendedDataOffset();
  void WriteAppendedDataOffset(
    vtkTypeInt64 streamPos, vtkTypeInt64& lastoffset, const char* attr = nullptr);
  void ForwardAppendedDataOffset(
    vtkTypeInt64 streamPos, vtkTypeInt64 offset, const char* attr = nullptr);
  void ForwardAppendedDataDouble(vtkTypeInt64 streamPos, double value, const char* attr);

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

  // Returns true if any keys were written.
  bool WriteInformation(vtkInformation* info, vtkIndent indent);

  void WriteArrayHeader(vtkAbstractArray* a, vtkIndent indent, const char* alternateName,
    int writeNumTuples, int timestep);
  virtual void WriteArrayFooter(
    ostream& os, vtkIndent indent, vtkAbstractArray* a, int shortFormat);
  virtual void WriteArrayInline(vtkAbstractArray* a, vtkIndent indent,
    const char* alternateName = nullptr, int writeNumTuples = 0);
  virtual void WriteInlineData(vtkAbstractArray* a, vtkIndent indent);

  void WriteArrayAppended(vtkAbstractArray* a, vtkIndent indent, OffsetsManager& offs,
    const char* alternateName = nullptr, int writeNumTuples = 0, int timestep = 0);
  int WriteAsciiData(vtkAbstractArray* a, vtkIndent indent);
  int WriteBinaryData(vtkAbstractArray* a);
  int WriteBinaryDataInternal(vtkAbstractArray* a);
  void WriteArrayAppendedData(vtkAbstractArray* a, vtkTypeInt64 pos, vtkTypeInt64& lastoffset);

  // Methods for writing points, point data, and cell data.
  void WriteFieldData(vtkIndent indent);
  void WriteFieldDataInline(vtkFieldData* fd, vtkIndent indent);
  void WritePointDataInline(vtkPointData* pd, vtkIndent indent);
  void WriteCellDataInline(vtkCellData* cd, vtkIndent indent);
  void WriteFieldDataAppended(vtkFieldData* fd, vtkIndent indent, OffsetsManagerGroup* fdManager);
  void WriteFieldDataAppendedData(vtkFieldData* fd, int timestep, OffsetsManagerGroup* fdManager);
  void WritePointDataAppended(vtkPointData* pd, vtkIndent indent, OffsetsManagerGroup* pdManager);
  void WritePointDataAppendedData(vtkPointData* pd, int timestep, OffsetsManagerGroup* pdManager);
  void WriteCellDataAppended(vtkCellData* cd, vtkIndent indent, OffsetsManagerGroup* cdManager);
  void WriteCellDataAppendedData(vtkCellData* cd, int timestep, OffsetsManagerGroup* cdManager);
  void WriteAttributeIndices(vtkDataSetAttributes* dsa, char** names);
  void WritePointsAppended(vtkPoints* points, vtkIndent indent, OffsetsManager* manager);
  void WritePointsAppendedData(vtkPoints* points, int timestep, OffsetsManager* pdManager);
  void WritePointsInline(vtkPoints* points, vtkIndent indent);
  void WriteCoordinatesInline(
    vtkDataArray* xc, vtkDataArray* yc, vtkDataArray* zc, vtkIndent indent);
  void WriteCoordinatesAppended(vtkDataArray* xc, vtkDataArray* yc, vtkDataArray* zc,
    vtkIndent indent, OffsetsManagerGroup* coordManager);
  void WriteCoordinatesAppendedData(vtkDataArray* xc, vtkDataArray* yc, vtkDataArray* zc,
    int timestep, OffsetsManagerGroup* coordManager);
  void WritePPointData(vtkPointData* pd, vtkIndent indent);
  void WritePCellData(vtkCellData* cd, vtkIndent indent);
  void WritePPoints(vtkPoints* points, vtkIndent indent);
  void WritePArray(vtkAbstractArray* a, vtkIndent indent, const char* alternateName = nullptr);
  void WritePCoordinates(vtkDataArray* xc, vtkDataArray* yc, vtkDataArray* zc, vtkIndent indent);

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

  // This shallows copy input field data to the passed field data and
  // then adds any additional field arrays. For example, TimeValue.
  void UpdateFieldData(vtkFieldData*);

  ostream* OutFile;
  std::ostringstream* OutStringStream;

  int OpenStream();
  int OpenFile();
  int OpenString();
  void CloseStream();
  void CloseFile();
  void CloseString();

  // The timestep currently being written
  int CurrentTimeIndex;
  int NumberOfTimeSteps;

  // Dummy boolean var to start/stop the continue executing:
  // when using the Start/Stop/WriteNextTime API
  int UserContinueExecuting; // can only be -1 = invalid, 0 = stop, 1 = start

  vtkTypeInt64* NumberOfTimeValues; // one per piece / per timestep

  friend class vtkXMLWriterHelper;

private:
  vtkXMLWriter(const vtkXMLWriter&) = delete;
  void operator=(const vtkXMLWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
