/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLWriter.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkBase64OutputStream.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkOutputStream.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkZLibDataCompressor.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef  vtkXMLOffsetsManager_DoNotInclude
#define vtkXMLDataHeaderPrivate_DoNotInclude
#include "vtkXMLDataHeaderPrivate.h"
#undef vtkXMLDataHeaderPrivate_DoNotInclude
#include "vtkXMLDataElement.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkQuadratureSchemeDefinition.h"

#include <vtksys/auto_ptr.hxx>
#include <vtksys/ios/sstream>

#include <assert.h>
#include <string>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

#if defined(__BORLANDC__)
#include <ctype.h> // isalnum is defined here for some versions of Borland
#endif

#include <locale> // C++ locale


//*****************************************************************************
// Friend class to enable access for  template functions to the protected
// writer methods.
class vtkXMLWriterHelper
{
public:
 static inline void SetProgressPartial(vtkXMLWriter* writer, double progress)
   {
   writer->SetProgressPartial(progress);
   }
 static inline int WriteBinaryDataBlock(vtkXMLWriter* writer,
   unsigned char* in_data, size_t numWords, int wordType)
   {
   return writer->WriteBinaryDataBlock(in_data, numWords, wordType);
   }
 static inline void* GetInt32IdTypeBuffer(vtkXMLWriter* writer)
   {
   return static_cast<void*>(writer->Int32IdTypeBuffer);
   }
 static inline unsigned char* GetByteSwapBuffer(vtkXMLWriter* writer)
   {
   return writer->ByteSwapBuffer;
   }
};

//----------------------------------------------------------------------------
template <class iterT>
int vtkXMLWriterWriteBinaryDataBlocks(vtkXMLWriter* writer,
  iterT* iter,
  int wordType, size_t memWordSize, size_t outWordSize)
{
  // generic implementation for fixed component length arrays.
  size_t numWords = iter->GetNumberOfValues();
  size_t blockWords = writer->GetBlockSize()/outWordSize;
  size_t memBlockSize = blockWords*memWordSize;

  // Prepare a pointer and counter to move through the data.
  unsigned char* ptr = reinterpret_cast<unsigned char*>(iter->GetTuple(0));
  size_t wordsLeft = numWords;

  // Do the complete blocks.
  vtkXMLWriterHelper::SetProgressPartial(writer, 0);
  int result = 1;
  while(result && (wordsLeft >= blockWords))
    {
    if(!vtkXMLWriterHelper::WriteBinaryDataBlock(writer, ptr, blockWords, wordType))
      {
      result = 0;
      }
    ptr += memBlockSize;
    wordsLeft -= blockWords;
    vtkXMLWriterHelper::SetProgressPartial(writer,
      float(numWords-wordsLeft)/numWords);
    }

  // Do the last partial block if any.
  if(result && (wordsLeft > 0))
    {
    if(!vtkXMLWriterHelper::WriteBinaryDataBlock(writer, ptr, wordsLeft, wordType))
      {
      result = 0;
      }
    }
  vtkXMLWriterHelper::SetProgressPartial(writer, 1);
  return result;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
int vtkXMLWriterWriteBinaryDataBlocks(vtkXMLWriter* writer,
  vtkArrayIteratorTemplate<vtkStdString>* iter,
  int wordType, size_t vtkNotUsed(memWordSize), size_t outWordSize)
{
  vtkXMLWriterHelper::SetProgressPartial(writer, 0);
  vtkStdString::value_type* allocated_buffer = 0;
  vtkStdString::value_type* temp_buffer = 0;
  if (vtkXMLWriterHelper::GetInt32IdTypeBuffer(writer))
    {
    temp_buffer = reinterpret_cast<vtkStdString::value_type*>(
      vtkXMLWriterHelper::GetInt32IdTypeBuffer(writer));
    }
  else if (vtkXMLWriterHelper::GetByteSwapBuffer(writer))
    {
    temp_buffer = reinterpret_cast<vtkStdString::value_type*>(
      vtkXMLWriterHelper::GetByteSwapBuffer(writer));
    }
  else
    {
    allocated_buffer = new vtkStdString::value_type[writer->GetBlockSize()/outWordSize];
    temp_buffer = allocated_buffer;
    }

  // For string arrays, writing as binary requires that the strings are written
  // out into a contiguous block. This is essential since the compressor can
  // only compress complete blocks of data.
  size_t numStrings = iter->GetNumberOfValues();
  size_t maxCharsPerBlock = writer->GetBlockSize() / outWordSize;

  size_t index = 0; // index in string array.
  int result = 1;
  vtkIdType stringOffset = 0; // num of chars of string written in pervious block.
    // this is required since a string may not fit completely in a block.

  while (result && index < numStrings) // write one block at a time.
    {
    size_t cur_offset = 0; // offset into the temp_buffer.
    while (index < numStrings && cur_offset < maxCharsPerBlock)
      {
      vtkStdString &str = iter->GetValue(index);
      vtkStdString::size_type length = str.size();
      const char* data = str.c_str();
      data += stringOffset; // advance by the chars already written.
      length -= stringOffset;
      stringOffset = 0;
      if (length == 0)
        {
        // just write the string termination char.
        temp_buffer[cur_offset++] = 0x0;
        }
      else
        {
        size_t new_offset = cur_offset + length + 1; // (+1) for termination char.
        if (new_offset <= maxCharsPerBlock)
          {
          memcpy(&temp_buffer[cur_offset], data, length);
          cur_offset += length;
          temp_buffer[cur_offset++] = 0x0;
          }
        else
          {
          size_t bytes_to_copy =  (maxCharsPerBlock - cur_offset);
          stringOffset = bytes_to_copy;
          memcpy(&temp_buffer[cur_offset], data, bytes_to_copy);
          cur_offset += bytes_to_copy;
          }
        }
      index++;
      }
    if (cur_offset > 0)
      {
      // We have a block of data to write.
      result = vtkXMLWriterHelper::WriteBinaryDataBlock(writer,
        reinterpret_cast<unsigned char*>(temp_buffer),
        cur_offset, wordType);
      vtkXMLWriterHelper::SetProgressPartial(writer, float(index)/numStrings);
      }
    }

  if (allocated_buffer)
    {
    delete [] allocated_buffer;
    allocated_buffer = 0;
    }
  vtkXMLWriterHelper::SetProgressPartial(writer, 1);
  return result;
}
//*****************************************************************************

vtkCxxSetObjectMacro(vtkXMLWriter, Compressor, vtkDataCompressor);
//----------------------------------------------------------------------------
vtkXMLWriter::vtkXMLWriter()
{
  this->FileName = 0;
  this->Stream = 0;

  // Default binary data mode is base-64 encoding.
  this->DataStream = vtkBase64OutputStream::New();

  // Byte order defaults to that of machine.
#ifdef VTK_WORDS_BIGENDIAN
  this->ByteOrder = vtkXMLWriter::BigEndian;
#else
  this->ByteOrder = vtkXMLWriter::LittleEndian;
#endif
  this->HeaderType = vtkXMLWriter::UInt32;

  // Output vtkIdType size defaults to real size.
#ifdef VTK_USE_64BIT_IDS
  this->IdType = vtkXMLWriter::Int64;
#else
  this->IdType = vtkXMLWriter::Int32;
#endif

  // Initialize compression data.
  this->BlockSize = 32768; //2^15
  this->Compressor = vtkZLibDataCompressor::New();
  this->CompressionHeader = 0;
  this->Int32IdTypeBuffer = 0;
  this->ByteSwapBuffer = 0;

  this->EncodeAppendedData = 1;
  this->AppendedDataPosition = 0;
  this->DataMode = vtkXMLWriter::Appended;
  this->ProgressRange[0] = 0;
  this->ProgressRange[1] = 1;

  this->SetNumberOfOutputPorts(0);
  this->SetNumberOfInputPorts(1);

  this->OutFile = 0;

  // Time support
  this->TimeStep = 0; // By default the file does not have timestep
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->UserContinueExecuting = -1; //invalid state
  this->NumberOfTimeValues = NULL;
  this->FieldDataOM = new OffsetsManagerGroup;
}

//----------------------------------------------------------------------------
vtkXMLWriter::~vtkXMLWriter()
{
  this->SetFileName(0);
  this->DataStream->Delete();
  this->SetCompressor(0);
  delete this->OutFile;

  delete this->FieldDataOM;
  delete[] this->NumberOfTimeValues;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetCompressorType(int compressorType)
{
  if (compressorType == NONE)
    {
    if (this->Compressor)
      {
      this->Compressor->Delete();
      this->Compressor = 0;
      this->Modified();
      }
    return;
    }

  if (compressorType == ZLIB)
    {
    if (this->Compressor && !this->Compressor->IsTypeOf("vtkZLibDataCompressor"))
      {
      this->Compressor->Delete();
      }

    this->Compressor = vtkZLibDataCompressor::New();
    this->Modified();
    return;
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    os << indent << "ByteOrder: BigEndian\n";
    }
  else
    {
    os << indent << "ByteOrder: LittleEndian\n";
    }
  if(this->IdType == vtkXMLWriter::Int32)
    {
    os << indent << "IdType: Int32\n";
    }
  else
    {
    os << indent << "IdType: Int64\n";
    }
  if(this->DataMode == vtkXMLWriter::Ascii)
    {
    os << indent << "DataMode: Ascii\n";
    }
  else if(this->DataMode == vtkXMLWriter::Binary)
    {
    os << indent << "DataMode: Binary\n";
    }
  else
    {
    os << indent << "DataMode: Appended\n";
    }
  if(this->Compressor)
    {
    os << indent << "Compressor: " << this->Compressor << "\n";
    }
  else
    {
    os << indent << "Compressor: (none)\n";
    }
  os << indent << "EncodeAppendedData: " << this->EncodeAppendedData << "\n";
  os << indent << "BlockSize: " << this->BlockSize << "\n";
  if(this->Stream)
    {
    os << indent << "Stream: " << this->Stream << "\n";
    }
  else
    {
    os << indent << "Stream: (none)\n";
    }
  os << indent << "TimeStep:" << this->TimeStep << "\n";
  os << indent << "NumberOfTimeSteps:" << this->NumberOfTimeSteps << "\n";
  os << indent << "TimeStepRange:(" << this->TimeStepRange[0] << ","
                                    << this->TimeStepRange[1] << ")\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXMLWriter::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetByteOrderToBigEndian()
{
  this->SetByteOrder(vtkXMLWriter::BigEndian);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetByteOrderToLittleEndian()
{
  this->SetByteOrder(vtkXMLWriter::LittleEndian);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetHeaderType(int t)
{
  if(t != vtkXMLWriter::UInt32 &&
     t != vtkXMLWriter::UInt64)
    {
    vtkErrorMacro(<< this->GetClassName() << " (" << this
                  << "): cannot set HeaderType to " << t);
    return;
    }
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting HeaderType to " << t);
  if(this->HeaderType != t)
    {
    this->HeaderType = t;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetHeaderTypeToUInt32()
{
  this->SetHeaderType(vtkXMLWriter::UInt32);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetHeaderTypeToUInt64()
{
  this->SetHeaderType(vtkXMLWriter::UInt64);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdType(int t)
{
#if !defined(VTK_USE_64BIT_IDS)
  if(t == vtkXMLWriter::Int64)
    {
    vtkErrorMacro("Support for Int64 vtkIdType not compiled in VTK.");
    return;
    }
#endif
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting IdType to " << t);
  if(this->IdType != t)
    {
    this->IdType = t;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdTypeToInt32()
{
  this->SetIdType(vtkXMLWriter::Int32);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdTypeToInt64()
{
  this->SetIdType(vtkXMLWriter::Int64);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToAscii()
{
  this->SetDataMode(vtkXMLWriter::Ascii);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToBinary()
{
  this->SetDataMode(vtkXMLWriter::Binary);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToAppended()
{
  this->SetDataMode(vtkXMLWriter::Appended);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetBlockSize(size_t blockSize)
{
  // Enforce constraints on block size.
  size_t nbs = blockSize;
#if VTK_SIZEOF_DOUBLE > VTK_SIZEOF_ID_TYPE
  typedef double LargestScalarType;
#else
  typedef vtkIdType LargestScalarType;
#endif
  size_t remainder = nbs % sizeof(LargestScalarType);
  if(remainder)
    {
    nbs -= remainder;
    if(nbs < sizeof(LargestScalarType))
      {
      nbs = sizeof(LargestScalarType);
      }
    vtkWarningMacro("BlockSize must be a multiple of "
                    << int(sizeof(LargestScalarType))
                    << ".  Using " << nbs << " instead of " << blockSize
                    << ".");
    }
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting BlockSize to " << nbs);
  if(this->BlockSize != nbs)
    {
    this->BlockSize = nbs;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::ProcessRequest(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::RequestData(vtkInformation* vtkNotUsed( request ),
                              vtkInformationVector** vtkNotUsed( inputVector ) ,
                              vtkInformationVector* vtkNotUsed( outputVector) )
{
  this->SetErrorCode(vtkErrorCode::NoError);

  // Make sure we have a file to write.
  if(!this->Stream && !this->FileName)
    {
    vtkErrorMacro("Writer called with no FileName set.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
    }

  // We are just starting to write.  Do not call
  // UpdateProgressDiscrete because we want a 0 progress callback the
  // first time.
  this->UpdateProgress(0);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = {0,1};
  this->SetProgressRange(wholeProgressRange, 0, 1);

  // Check input validity and call the real writing code.
  int result = this->WriteInternal();

  // If writing failed, delete the file.
  if(!result)
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->DeleteAFile();
    }

  // We have finished writing.
  this->UpdateProgressDiscrete(1);

  return result;
}
//----------------------------------------------------------------------------
int vtkXMLWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }

  // always write even if the data hasn't changed
  this->Modified();

  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::OpenFile()
{
  this->OutFile = 0;
  if(this->Stream)
    {
    // Rewind stream to the beginning.
    this->Stream->seekp(0);
    }
  else
    {
    // Strip trailing whitespace from the filename.
    int len = static_cast<int>(strlen(this->FileName));
    for (int i = len-1; i >= 0; i--)
      {
      if (isalnum(this->FileName[i]))
        {
        break;
        }
      this->FileName[i] = 0;
      }

    // Try to open the output file for writing.
#ifdef _WIN32
    this->OutFile = new ofstream(this->FileName, ios::out | ios::binary);
#else
    this->OutFile = new ofstream(this->FileName, ios::out);
#endif
    if(!this->OutFile || !*this->OutFile)
      {
      vtkErrorMacro("Error opening output file \"" << this->FileName << "\"");
      this->SetErrorCode(vtkErrorCode::GetLastSystemError());
      vtkErrorMacro("Error code \""
                    << vtkErrorCode::GetStringFromErrorCode(this->GetErrorCode()) << "\"");
      return 0;
      }
    this->Stream = this->OutFile;
    }

  // Make sure sufficient precision is used in the ascii
  // representation of data and meta-data.
  this->Stream->precision(11);

  // Setup the output streams.
  this->DataStream->SetStream(this->Stream);

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::CloseFile()
{
  // Cleanup the output streams.
  this->DataStream->SetStream(0);

  if(this->OutFile)
    {
    // We opened a file.  Close it.
    delete this->OutFile;
    this->OutFile = 0;
    this->Stream = 0;
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteInternal()
{
  if (!this->OpenFile())
    {
    return 0;
    }

  (*this->Stream).imbue(std::locale::classic());

  // Tell the subclass to write the data.
  int result = this->WriteData();

  // if user manipulate execution don't try closing file
  if( this->UserContinueExecuting != 1 )
    {
    this->CloseFile();
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::GetDataSetMajorVersion()
{
  if(this->HeaderType == vtkXMLWriter::UInt64)
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::GetDataSetMinorVersion()
{
  if(this->HeaderType == vtkXMLWriter::UInt64)
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLWriter::GetInputAsDataSet()
{
  return static_cast<vtkDataSet*>(this->GetInput());
}

//----------------------------------------------------------------------------
int vtkXMLWriter::StartFile()
{
  ostream& os = *(this->Stream);

  // If this will really be a valid XML file, put the XML header at
  // the top.
  if(this->EncodeAppendedData)
    {
    os << "<?xml version=\"1.0\"?>\n";
    }

  os.imbue(std::locale::classic());

  // Open the document-level element.  This will contain the rest of
  // the elements.
  os << "<VTKFile";
  this->WriteFileAttributes();
  os << ">\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFileAttributes()
{
  ostream& os = *(this->Stream);

  // Write the file's type.
  this->WriteStringAttribute("type", this->GetDataSetName());

  // Write the version number of the file.
  os << " version=\""
     << this->GetDataSetMajorVersion()
     << "."
     << this->GetDataSetMinorVersion()
     << "\"";

  // Write the byte order for the file.
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    os << " byte_order=\"BigEndian\"";
    }
  else
    {
    os << " byte_order=\"LittleEndian\"";
    }

  // Write the header type for binary data.
  if(this->HeaderType == vtkXMLWriter::UInt64)
    {
    os << " header_type=\"UInt64\"";
    }
#if 0 // future: else if(this->FileMajorVersion >= 1)
    {
    os << " header_type=\"UInt32\"";
    }
#endif

  // Write the compressor that will be used for the file.
  if(this->Compressor)
    {
    os << " compressor=\"" << this->Compressor->GetClassName() << "\"";
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::EndFile()
{
  ostream& os = *(this->Stream);

  // Close the document-level element.
  os << "</VTKFile>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::DeleteAFile()
{
  if(!this->Stream && this->FileName)
    {
    this->DeleteAFile(this->FileName);
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::DeleteAFile(const char* name)
{
  unlink(name);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::StartAppendedData()
{
  ostream& os = *(this->Stream);
  os << "  <AppendedData encoding=\""
     << (this->EncodeAppendedData? "base64" : "raw")
     << "\">\n";
  os << "   _";
  this->AppendedDataPosition = os.tellp();

  // Setup proper output encoding.
  if(this->EncodeAppendedData)
    {
    vtkBase64OutputStream* base64 = vtkBase64OutputStream::New();
    this->SetDataStream(base64);
    base64->Delete();
    }
  else
    {
    vtkOutputStream* raw = vtkOutputStream::New();
    this->SetDataStream(raw);
    raw->Delete();
    }

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::EndAppendedData()
{
  ostream& os = *(this->Stream);
  os << "\n";
  os << "  </AppendedData>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
vtkTypeInt64
vtkXMLWriter::ReserveAttributeSpace(const char* attr, size_t length)
{
  // Save the starting stream position.
  ostream& os = *(this->Stream);
  vtkTypeInt64 startPosition = os.tellp();

  // By default write an empty valid xml: attr="".  In most case it
  // will be overwritten but we guarantee that the xml produced will
  // be valid in case we stop writing too early.
  os << " " << attr << "=\"\"";

  // Now reserve space for the value.
  for(size_t i=0; i < length; ++i)
    {
    os << " ";
    }

  // Flush the stream to make sure the system tries to write now and
  // test for a write error reported by the system.
  os.flush();
  if(os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  // Return the position at which to write the attribute later.
  return startPosition;
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkXMLWriter::GetAppendedDataOffset()
{
  vtkTypeInt64 pos = this->Stream->tellp();
  return (pos - this->AppendedDataPosition);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteAppendedDataOffset(vtkTypeInt64 streamPos,
                                           vtkTypeInt64 &lastoffset,
                                           const char* attr)
{
  // Write an XML attribute with the given name.  The value is the
  // current appended data offset.  Starts writing at the given stream
  // position, and returns the ending position.  If attr is 0, writes
  // only the double quotes.  In all cases, the final stream position
  // is left the same as before the call.
  ostream& os = *(this->Stream);
  vtkTypeInt64 returnPos = os.tellp();
  vtkTypeInt64 offset = returnPos - this->AppendedDataPosition;
  lastoffset = offset; //saving result
  os.seekp(std::streampos(streamPos));
  if(attr)
    {
    os << " " << attr << "=";
    }
  os << "\"" << offset << "\"";
  os.seekp(std::streampos(returnPos));

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::ForwardAppendedDataOffset(
  vtkTypeInt64 streamPos, vtkTypeInt64 offset, const char* attr)
{
  ostream& os = *(this->Stream);
  std::streampos returnPos = os.tellp();
  os.seekp(std::streampos(streamPos));
  if(attr)
    {
    os << " " << attr << "=";
    }
  os << "\"" << offset << "\"";
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::ForwardAppendedDataDouble(
  vtkTypeInt64 streamPos, double value, const char* attr)
{
  ostream& os = *(this->Stream);
  std::streampos returnPos = os.tellp();
  os.seekp(std::streampos(streamPos));
  if(attr)
    {
    os << " " << attr << "=";
    }
  os << "\"" << value << "\"";
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryData(vtkAbstractArray* a)
{
  int wordType = a->GetDataType();
  size_t outWordSize = this->GetOutputWordTypeSize(wordType);
  size_t data_size = a->GetDataSize();
  if(this->Compressor)
    {
    // Need to compress the data.  Create compression header.  This
    // reserves enough space in the output.
    if(!this->CreateCompressionHeader(data_size*outWordSize))
      {
      return 0;
      }
    // Start writing the data.
    int result = this->DataStream->StartWriting();

    // Process the actual data.
    if (result && !this->WriteBinaryDataInternal(a))
      {
      result = 0;
      }

    // Finish writing the data.
    if(result && !this->DataStream->EndWriting())
      {
      result = 0;
      }

    // Go back and write the real compression header in its proper place.
    if(result && !this->WriteCompressionHeader())
      {
      result = 0;
      }

    // Destroy the compression header if it was used.
    if(this->CompressionHeader)
      {
      delete this->CompressionHeader;
      this->CompressionHeader = 0;
      }

    return result;
    }
  else
    {
    // Start writing the data.
    if(!this->DataStream->StartWriting())
      {
      return 0;
      }

    // No data compression.  The header is just the length of the data.
    vtksys::auto_ptr<vtkXMLDataHeader>
      uh(vtkXMLDataHeader::New(this->HeaderType, 1));
    if(!uh->Set(0, data_size*outWordSize))
      {
      vtkErrorMacro("Array \"" << a->GetName() <<
                    "\" is too large.  Set HeaderType to UInt64.");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return 0;
      }
    this->PerformByteSwap(uh->Data(), uh->WordCount(), uh->WordSize());
    int writeRes = this->DataStream->Write(uh->Data(), uh->DataSize());
    this->Stream->flush();
    if (this->Stream->fail())
      {
      this->SetErrorCode(vtkErrorCode::GetLastSystemError());
      return 0;
      }
    if(!writeRes)
      {
      return 0;
      }

    // Process the actual data.
    if(!this->WriteBinaryDataInternal(a))
      {
      return 0;
      }

    // Finish writing the data.
    if(!this->DataStream->EndWriting())
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryDataInternal(vtkAbstractArray* a)
{
  // Break into blocks and handle each one separately.  This allows
  // for better random access when reading compressed data and saves
  // memory during writing.

  // The size of the blocks written (before compression) is
  // this->BlockSize.  We need to support the possibility that the
  // size of data in memory and the size on disk are different.  This
  // is necessary to allow vtkIdType to be converted to UInt32 for
  // writing.

  int wordType = a->GetDataType();
  size_t memWordSize = this->GetWordTypeSize(wordType);
  size_t outWordSize = this->GetOutputWordTypeSize(wordType);

#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    size_t blockWordsEstimate = this->BlockSize / outWordSize;
    this->Int32IdTypeBuffer = new Int32IdType[blockWordsEstimate];
    }
#endif

  // Decide if we need to byte swap.
#ifdef VTK_WORDS_BIGENDIAN
  if(outWordSize > 1 && this->ByteOrder != vtkXMLWriter::BigEndian)
#else
  if(outWordSize > 1 && this->ByteOrder != vtkXMLWriter::LittleEndian)
#endif
    {
    // We need to byte swap.  Prepare a buffer large enough for one
    // block.
    if(this->Int32IdTypeBuffer)
      {
      // Just swap in-place in the converted id-type buffer.
      this->ByteSwapBuffer =
        reinterpret_cast<unsigned char*>(this->Int32IdTypeBuffer);
      }
    else
      {
      // The maximum nlock size if this->BlockSize. The actual data in the block
      // may be lesser.
      this->ByteSwapBuffer = new unsigned char[this->BlockSize];
      }
    }
  int ret;;
  vtkArrayIterator* iter = a->NewIterator();
  switch (wordType)
    {
    vtkArrayIteratorTemplateMacro(
      ret = vtkXMLWriterWriteBinaryDataBlocks(this,
        static_cast<VTK_TT*>(iter),
        wordType, memWordSize, outWordSize));
  default:
    vtkWarningMacro("Cannot write binary data of type : " << wordType);
    ret = 0;
    }
  iter->Delete();

  // Free the byte swap buffer if it was allocated.
  if(this->ByteSwapBuffer && !this->Int32IdTypeBuffer)
    {
    delete [] this->ByteSwapBuffer;
    this->ByteSwapBuffer = 0;
    }

#ifdef VTK_USE_64BIT_IDS
  // Free the id-type conversion buffer if it was allocated.
  if(this->Int32IdTypeBuffer)
    {
    delete [] this->Int32IdTypeBuffer;
    this->Int32IdTypeBuffer = 0;
    }
#endif
  return ret;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryDataBlock(unsigned char* in_data,
                                       size_t numWords, int wordType)
{
  unsigned char* data = in_data;
#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    vtkIdType* idBuffer = reinterpret_cast<vtkIdType*>(in_data);

    for(size_t i=0;i < numWords; ++i)
      {
      this->Int32IdTypeBuffer[i] = static_cast<Int32IdType>(idBuffer[i]);
      }

    data = reinterpret_cast<unsigned char*>(this->Int32IdTypeBuffer);
    }
#endif

  // Get the word size of the data buffer.  This is now the size that
  // will be written.
  size_t wordSize = this->GetOutputWordTypeSize(wordType);

  // If we need to byte swap, do it now.
  if(this->ByteSwapBuffer)
    {
    // If we are converting vtkIdType to 32-bit integer data, the data
    // are already in the byte swap buffer because we share the
    // conversion buffer.  Otherwise, we need to copy the data before
    // byte swapping.
    if(data != this->ByteSwapBuffer)
      {
      memcpy(this->ByteSwapBuffer, data, numWords*wordSize);
      data = this->ByteSwapBuffer;
      }
    this->PerformByteSwap(this->ByteSwapBuffer, numWords, wordSize);
    }

  // Now pass the data to the next write phase.
  if(this->Compressor)
    {
    int res = this->WriteCompressionBlock(data, numWords*wordSize);
    this->Stream->flush();
    if (this->Stream->fail())
      {
      this->SetErrorCode(vtkErrorCode::GetLastSystemError());
      return 0;
      }
    return res;
    }
  else
    {
    int res = this->DataStream->Write(data, numWords*wordSize);
    this->Stream->flush();
    if (this->Stream->fail())
      {
      this->SetErrorCode(vtkErrorCode::GetLastSystemError());
      return 0;
      }
    return res;
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::PerformByteSwap(void* data, size_t numWords,
                                   size_t wordSize)
{
  char* ptr = static_cast<char*>(data);
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2BERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4BERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8BERange(ptr, numWords); break;
      default:
        vtkErrorMacro("Unsupported data type size " << wordSize);
      }
    }
  else
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2LERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4LERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8LERange(ptr, numWords); break;
      default:
        vtkErrorMacro("Unsupported data type size " << wordSize);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataStream(vtkOutputStream* arg)
{
  if(this->DataStream != arg)
    {
    if(this->DataStream != NULL)
      {
      this->DataStream->UnRegister(this);
      }
    this->DataStream = arg;
    if(this->DataStream != NULL)
      {
      this->DataStream->Register(this);
      this->DataStream->SetStream(this->Stream);
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::CreateCompressionHeader(size_t size)
{
  // Allocate and initialize the compression header.
  // The format is this:
  //  struct header {
  //    HeaderType number_of_blocks;
  //    HeaderType uncompressed_block_size;
  //    HeaderType uncompressed_last_block_size;
  //    HeaderType compressed_block_sizes[number_of_blocks];
  //  }

  // Find the size and number of blocks.
  size_t numFullBlocks = size / this->BlockSize;
  size_t lastBlockSize = size % this->BlockSize;
  size_t numBlocks = numFullBlocks + (lastBlockSize?1:0);
  this->CompressionHeader =
    vtkXMLDataHeader::New(this->HeaderType, 3+numBlocks);

  // Write out dummy header data.
  this->CompressionHeaderPosition = this->Stream->tellp();
  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(this->CompressionHeader->Data(),
                                        this->CompressionHeader->DataSize()) &&
                this->DataStream->EndWriting());

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  // Fill in known header data now.
  this->CompressionHeader->Set(0, numBlocks);
  this->CompressionHeader->Set(1, this->BlockSize);
  this->CompressionHeader->Set(2, lastBlockSize);

  // Initialize counter for block writing.
  this->CompressionBlockNumber = 0;

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionBlock(unsigned char* data, size_t size)
{
  // Compress the data.
  vtkUnsignedCharArray* outputArray = this->Compressor->Compress(data, size);

  // Find the compressed size.
  size_t outputSize = outputArray->GetNumberOfTuples();
  unsigned char* outputPointer = outputArray->GetPointer(0);

  // Write the compressed data.
  int result = this->DataStream->Write(outputPointer, outputSize);
  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  // Store the resulting compressed size in the compression header.
  this->CompressionHeader->Set(3+this->CompressionBlockNumber++, outputSize);

  outputArray->Delete();

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionHeader()
{
  // Write real compression header back into stream.
  std::streampos returnPosition = this->Stream->tellp();

  // Need to byte-swap header.
  this->PerformByteSwap(this->CompressionHeader->Data(),
                        this->CompressionHeader->WordCount(),
                        this->CompressionHeader->WordSize());

  if(!this->Stream->seekp(std::streampos(this->CompressionHeaderPosition)))
    { return 0; }
  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(this->CompressionHeader->Data(),
                                        this->CompressionHeader->DataSize()) &&
                this->DataStream->EndWriting());
  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  if(!this->Stream->seekp(returnPosition)) { return 0; }
  return result;
}

//----------------------------------------------------------------------------
size_t vtkXMLWriter::GetOutputWordTypeSize(int dataType)
{
#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
  if((dataType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    return 4;
    }
#endif
  return this->GetWordTypeSize(dataType);
}

//----------------------------------------------------------------------------
template <class T>
size_t vtkXMLWriterGetWordTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
size_t vtkXMLWriter::GetWordTypeSize(int dataType)
{
  size_t size = 1;
  switch (dataType)
    {
    vtkTemplateMacro(
      size = vtkXMLWriterGetWordTypeSize(static_cast<VTK_TT*>(0))
      );
  case VTK_STRING:
    return sizeof(vtkStdString::value_type);

  default:
      { vtkWarningMacro("Unsupported data type: " << dataType); } break;
    }
  return size;
}

//----------------------------------------------------------------------------
const char* vtkXMLWriter::GetWordTypeName(int dataType)
{
  char isSigned = 0;
  int size = 0;

  // These string values must match vtkXMLDataElement::GetWordTypeAttribute().
  switch (dataType)
    {
  case VTK_STRING:           return "String";
    case VTK_FLOAT:          return "Float32";
    case VTK_DOUBLE:         return "Float64";
    case VTK_ID_TYPE:
      {
      switch (this->IdType)
        {
        case vtkXMLWriter::Int32: return "Int32";
        case vtkXMLWriter::Int64: return "Int64";
        default: return 0;
        }
      }
#if VTK_TYPE_CHAR_IS_SIGNED
    case VTK_CHAR:           isSigned = 1; size = sizeof(char); break;
#else
    case VTK_CHAR:           isSigned = 0; size = sizeof(char); break;
#endif
    case VTK_INT:            isSigned = 1; size = sizeof(int); break;
    case VTK_LONG:           isSigned = 1; size = sizeof(long); break;
    case VTK_SHORT:          isSigned = 1; size = sizeof(short); break;
    case VTK_SIGNED_CHAR:    isSigned = 1; size = sizeof(signed char); break;
    case VTK_UNSIGNED_CHAR:  isSigned = 0; size = sizeof(unsigned char); break;
    case VTK_UNSIGNED_INT:   isSigned = 0; size = sizeof(unsigned int); break;
    case VTK_UNSIGNED_LONG:  isSigned = 0; size = sizeof(unsigned long); break;
    case VTK_UNSIGNED_SHORT: isSigned = 0; size = sizeof(unsigned short); break;
#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:          isSigned = 1; size = sizeof(long long); break;
    case VTK_UNSIGNED_LONG_LONG: isSigned = 0; size = sizeof(unsigned long long); break;
#endif
#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:            isSigned = 1; size = sizeof(__int64); break;
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:   isSigned = 0; size = sizeof(unsigned __int64); break;
# endif
#endif
    default:
    {
    vtkWarningMacro("Unsupported data type: " << dataType); } break;
    }
  const char* type = 0;
  switch (size)
    {
    case 1: type = isSigned? "Int8"  : "UInt8";  break;
    case 2: type = isSigned? "Int16" : "UInt16"; break;
    case 4: type = isSigned? "Int32" : "UInt32"; break;
    case 8: type = isSigned? "Int64" : "UInt64"; break;
    default:
      {
      vtkErrorMacro("Data type size " << size
                    << " not supported by VTK XML format.");
      }
    }
  return type;
}

//----------------------------------------------------------------------------
template <class T>
int vtkXMLWriterWriteVectorAttribute(ostream& os, const char* name,
                                     int length, T* data)
{
  os << " " << name << "=\"";
  if(length)
    {
    int i;
    os << data[0];
    for(i=1;i < length; ++i)
      {
      os << " " << data[i];
      }
    }
  os << "\"";
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteScalarAttribute(const char* name, int data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteScalarAttribute(const char* name, float data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteScalarAttribute(const char* name, double data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkXMLWriter::WriteScalarAttribute(const char* name, vtkIdType data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}
#endif

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       int* data)
{
  int res =
    vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       float* data)
{
  int res =
    vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       double* data)
{
  int res =
    vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return res;
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       vtkIdType* data)
{
  int res =
    vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return res;
}
#endif

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteDataModeAttribute(const char* name)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"";
  if(this->DataMode == vtkXMLWriter::Appended)
    {
    os << "appended";
    }
  else if(this->DataMode == vtkXMLWriter::Binary)
    {
    os << "binary";
    }
  else
    {
    os << "ascii";
    }
  os << "\"";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteWordTypeAttribute(const char* name, int dataType)
{
  ostream& os = *(this->Stream);
  const char* value = this->GetWordTypeName(dataType);
  if(!value)
    {
    return 0;
    }
  os << " " << name << "=\"" << value << "\"";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteStringAttribute(const char* name, const char* value)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"" << value << "\"";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
// This method is provided so that the specialization code for certain types
// can be minimal.
template <class T>
inline ostream& vtkXMLWriteAsciiValue(ostream& os, const T& value)
{
  os << value;
  return os;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline ostream& vtkXMLWriteAsciiValue(ostream& os, const char &c)
{
  os << short(c);
  return os;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline ostream& vtkXMLWriteAsciiValue(ostream& os, const unsigned char &c)
{
  os << static_cast<unsigned short>(c);
  return os;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline ostream& vtkXMLWriteAsciiValue(ostream& os, const signed char &c)
{
  os << short(c);
  return os;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline ostream& vtkXMLWriteAsciiValue(ostream& os, const vtkStdString& str)
{
  vtkStdString::const_iterator iter = str.begin();
  vtkXMLWriteAsciiValue(os, *iter);
  iter++;
  for (; iter != str.end(); ++iter)
    {
    os << " ";
    vtkXMLWriteAsciiValue(os, *iter);
    }
  os << " ";
  char delim = 0x0;
  return vtkXMLWriteAsciiValue(os, delim);
}

//----------------------------------------------------------------------------
template <class iterT>
int vtkXMLWriteAsciiData(ostream& os, iterT* iter, vtkIndent indent)
{
  if (!iter)
    {
    return 0;
    }
  size_t columns = 6;
  size_t length = iter->GetNumberOfTuples() *
    iter->GetNumberOfComponents();

  size_t rows = length/columns;
  size_t lastRowLength = length%columns;
  size_t r,c;
  vtkIdType index=0;
  for(r=0;r < rows;++r)
    {
    os << indent;
    vtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    for(c=1;c < columns;++c)
      {
      os << " ";
      vtkXMLWriteAsciiValue(os, iter->GetValue(index++));
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent;
    vtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    for(c=1;c < lastRowLength;++c)
      {
      os << " " ;
    vtkXMLWriteAsciiValue(os, iter->GetValue(index++));
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteAsciiData(vtkAbstractArray* a, vtkIndent indent)
{
  vtkArrayIterator* iter = a->NewIterator();
  ostream& os = *(this->Stream);
  int ret;
  switch(a->GetDataType())
    {
    vtkArrayIteratorTemplateMacro(
      ret = vtkXMLWriteAsciiData(os,
        static_cast<VTK_TT*>(iter),
        indent));
    // Why isn;t vtkBitArray handled?
  default:
    ret = 0;
    break;
    }
  iter->Delete();
  return ret;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteArrayAppended(
        vtkAbstractArray* a,
        vtkIndent indent,
        OffsetsManager &offs,
        const char* alternateName,
        int writeNumTuples,
        int timestep)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray or <Array:
  this->WriteArrayHeader(a,indent,alternateName, writeNumTuples, timestep);
  int shortFormatTag=1; // close with: />
  //
  if (vtkDataArray::SafeDownCast(a))
    {
    // write the scalar range of this data array, we reserver space because we
    // don't actually have the data at this point
    offs.GetRangeMinPosition(timestep)
      = this->ReserveAttributeSpace("RangeMin");
    offs.GetRangeMaxPosition(timestep)
      = this->ReserveAttributeSpace("RangeMax");
    }
  else
    {
    // ranges are not written for non-data arrays.
    offs.GetRangeMinPosition(timestep) = -1;
    offs.GetRangeMaxPosition(timestep) = -1;
    }

  //
  offs.GetPosition(timestep) = this->ReserveAttributeSpace("offset");
  // Write information in the recognized keys associated with this array.
  vtkInformation *info=a->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey *key=vtkQuadratureSchemeDefinition::DICTIONARY();
  if (info->Has(key))
    {
    // Close the header
    os << ">" << endl;
    vtkXMLDataElement *eKey=vtkXMLDataElement::New();
    key->SaveState(info,eKey);
    eKey->PrintXML(os,indent.GetNextIndent());
    eKey->Delete();
    shortFormatTag=0; // close with </DataArray> or </Array>
    }
  // Close tag.
  this->WriteArrayFooter(os, indent, a, shortFormatTag);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteArrayAppendedData(vtkAbstractArray* a,
                                          vtkTypeInt64 pos,
                                          vtkTypeInt64& lastoffset)
{
  this->WriteAppendedDataOffset(pos, lastoffset, "offset");
  this->WriteBinaryData(a);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteArrayHeader(vtkAbstractArray* a,  vtkIndent indent,
                                        const char* alternateName,
                                        int writeNumTuples,
                                        int timestep)
{
  ostream& os = *(this->Stream);
  if (vtkDataArray::SafeDownCast(a))
    {
    os << indent << "<DataArray";
    }
  else
    {
    os << indent << "<Array";
    }
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if(alternateName)
    {
    this->WriteStringAttribute("Name", alternateName);
    }
  else if(const char* arrayName = a->GetName())
    {
    this->WriteStringAttribute("Name", arrayName);
    }
  else
    {
    // Generate a name for this array.
    vtksys_ios::ostringstream name;
    void* p = a;
    name << "Array " << p;
    this->WriteStringAttribute("Name", name.str().c_str());
    }
  if(a->GetNumberOfComponents() > 1)
    {
    this->WriteScalarAttribute("NumberOfComponents",
      a->GetNumberOfComponents());
    }

  //always write out component names, even if only 1 component
  vtksys_ios::ostringstream buff;
  const char* compName = NULL;
  for ( int i=0; i < a->GetNumberOfComponents(); ++i )
    {
    //get the component names
    buff << "ComponentName" << i;
    compName = a->GetComponentName( i );
    if ( compName )
      {
      this->WriteStringAttribute( buff.str().c_str(), compName );
      compName = NULL;
      }
    buff.str("");
    buff.clear();
    }

  if(this->NumberOfTimeSteps > 1)
    {
    this->WriteScalarAttribute("TimeStep", timestep);
    }
  else
    {
    //assert( timestep == -1); //FieldData problem
    }
  if(writeNumTuples)
    {
    this->WriteScalarAttribute("NumberOfTuples",
      a->GetNumberOfTuples());
    }

  this->WriteDataModeAttribute("format");
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteArrayFooter(
        ostream &os,
        vtkIndent indent,
        vtkAbstractArray *a,
        int shortFormat)
{
   // Close the tag: </DataArray>, </Array> or />
  if (shortFormat)
    {
    os << "/>" << endl;
    }
  else
    {
    vtkDataArray* da = vtkDataArray::SafeDownCast(a);
    if (da)
      {
      os << indent << "</DataArray>\n";
      }
    else
      {
      os << indent << "</Array>\n";
      }
    }
  // Force write and check for errors.
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteInlineData(vtkAbstractArray* a, vtkIndent indent)
{
  if(this->DataMode == vtkXMLWriter::Binary)
    {
    ostream& os = *(this->Stream);
    os << indent;
    this->WriteBinaryData(a);
    os << "\n";
    }
  else
    {
    this->WriteAsciiData(a, indent);
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteArrayInline(
        vtkAbstractArray* a,
        vtkIndent indent,
        const char* alternateName,
        int writeNumTuples)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray or <Array:
  this->WriteArrayHeader(a, indent, alternateName, writeNumTuples, 0);
  //
  vtkDataArray* da = vtkDataArray::SafeDownCast(a);
  if (da)
    {
    // write the range
    this->WriteScalarAttribute("RangeMin",da->GetRange(-1)[0]);
    this->WriteScalarAttribute("RangeMax",da->GetRange(-1)[1]);
    }
  // Close the header
  os << ">\n";
  // Write recognized information keys associated with this array.
  vtkInformation *info=a->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey *key=vtkQuadratureSchemeDefinition::DICTIONARY();
  if (info->Has(key))
    {
    vtkXMLDataElement *eKey=vtkXMLDataElement::New();
    key->SaveState(info,eKey);
    eKey->PrintXML(os,indent);
    eKey->Delete();
    }
  // Write the inline data.
  this->WriteInlineData(a, indent.GetNextIndent());
  // Close tag.
  this->WriteArrayFooter(os, indent, a, 0);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFieldData(vtkIndent indent)
{
  vtkFieldData *fieldData = this->GetInput()->GetFieldData();
  if (!fieldData || !fieldData->GetNumberOfArrays())
    {
    return;
    }

  if(this->DataMode == vtkXMLWriter::Appended)
    {
    this->WriteFieldDataAppended(fieldData, indent, this->FieldDataOM);
    }
  else
    {
    // Write the point data arrays.
    this->WriteFieldDataInline(fieldData, indent);
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFieldDataInline(vtkFieldData* fd, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(fd->GetNumberOfArrays());

  os << indent << "<FieldData>\n";

  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int i;
  for(i=0; i < fd->GetNumberOfArrays(); ++i)
    {
    this->SetProgressRange(progressRange, i, fd->GetNumberOfArrays());
    this->WriteArrayInline(fd->GetAbstractArray(i), indent.GetNextIndent(),
      names[i], 1);
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      this->DestroyStringArray(fd->GetNumberOfArrays(), names);
      return;
      }
    }

  os << indent << "</FieldData>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(fd->GetNumberOfArrays(), names);
    return;
    }

  this->DestroyStringArray(fd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointDataInline(vtkPointData* pd, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
    }

  os << ">\n";

  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    this->SetProgressRange(progressRange, i, pd->GetNumberOfArrays());
    vtkAbstractArray* a = this->CreateArrayForPoints(pd->GetAbstractArray(i));
    this->WriteArrayInline(a, indent.GetNextIndent(), names[i]);
    a->Delete();
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      this->DestroyStringArray(pd->GetNumberOfArrays(), names);
      return;
      }
    }

  os << indent << "</PointData>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
    }

  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCellDataInline(vtkCellData* cd, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
    }

  os << ">\n";

  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    this->SetProgressRange(progressRange, i, cd->GetNumberOfArrays());
    vtkAbstractArray* a = this->CreateArrayForCells(cd->GetAbstractArray(i));
    this->WriteArrayInline(a, indent.GetNextIndent(), names[i]);
    a->Delete();
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      this->DestroyStringArray(cd->GetNumberOfArrays(), names);
      return;
      }
    }

  os << indent << "</CellData>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
    }

  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}


//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFieldDataAppended(vtkFieldData* fd,
                                          vtkIndent indent,
                                          OffsetsManagerGroup *fdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(fd->GetNumberOfArrays());

  os << indent << "<FieldData>\n";

  int i;
  // When we want to write index arrays with String Arrays, we will
  // have to determine the actual arrays written out to the file
  // and allocate the fdManager accordingly.
  fdManager->Allocate(fd->GetNumberOfArrays());
  for(i=0; i < fd->GetNumberOfArrays(); ++i)
    {
    fdManager->GetElement(i).Allocate(1);
    this->WriteArrayAppended(fd->GetAbstractArray(i),
      indent.GetNextIndent(), fdManager->GetElement(i), names[i], 1 , 0);
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      this->DestroyStringArray(fd->GetNumberOfArrays(), names);
      return;
      }
    }
  os << indent << "</FieldData>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  this->DestroyStringArray(fd->GetNumberOfArrays(), names);
}



//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFieldDataAppendedData(vtkFieldData* fd, int timestep,
                                              OffsetsManagerGroup *fdManager)
{
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int i;
  fdManager->Allocate(fd->GetNumberOfArrays());
  for(i=0; i < fd->GetNumberOfArrays(); ++i)
    {
    fdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    this->SetProgressRange(progressRange, i, fd->GetNumberOfArrays());
    this->WriteArrayAppendedData(fd->GetAbstractArray(i),
      fdManager->GetElement(i).GetPosition(timestep),
      fdManager->GetElement(i).GetOffsetValue(timestep));
    vtkDataArray* da = fd->GetArray(i);
    if (da)
      {
      // Write ranges only for data arrays.
      double *range = da->GetRange(-1);
      this->ForwardAppendedDataDouble
        (fdManager->GetElement(i).GetRangeMinPosition(timestep),
         range[0],"RangeMin" );
      this->ForwardAppendedDataDouble
        (fdManager->GetElement(i).GetRangeMaxPosition(timestep),
         range[1],"RangeMax" );
      }
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    }
}


//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointDataAppended(vtkPointData* pd, vtkIndent indent,
                                          OffsetsManagerGroup *pdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
    }

  os << ">\n";

  pdManager->Allocate(pd->GetNumberOfArrays());
  for(int i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    pdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    for(int t=0; t< this->NumberOfTimeSteps; ++t)
      {
      this->WriteArrayAppended(pd->GetAbstractArray(i), indent.GetNextIndent(),
        pdManager->GetElement(i), names[i], 0, t);
      if (this->ErrorCode != vtkErrorCode::NoError)
        {
        this->DestroyStringArray(pd->GetNumberOfArrays(), names);
        return;
        }
      }
    }

  os << indent << "</PointData>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointDataAppendedData(vtkPointData* pd, int timestep,
                                              OffsetsManagerGroup *pdManager)
{
  float progressRange[2] = {0,0};

  this->GetProgressRange(progressRange);
  for(int i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    this->SetProgressRange(progressRange, i, pd->GetNumberOfArrays());
    unsigned long mtime = pd->GetMTime();
    // Only write pd if MTime has changed
    unsigned long &pdMTime = pdManager->GetElement(i).GetLastMTime();
    vtkAbstractArray* a = this->CreateArrayForPoints(pd->GetAbstractArray(i));
    if( pdMTime != mtime )
      {
      pdMTime = mtime;
      this->WriteArrayAppendedData( a,
         pdManager->GetElement(i).GetPosition(timestep),
         pdManager->GetElement(i).GetOffsetValue(timestep));
      if (this->ErrorCode != vtkErrorCode::NoError)
        {
        return;
        }
      }
    else
      {
      assert( timestep > 0 );
      pdManager->GetElement(i).GetOffsetValue(timestep) =
        pdManager->GetElement(i).GetOffsetValue(timestep-1);
      this->ForwardAppendedDataOffset
        (pdManager->GetElement(i).GetPosition(timestep),
         pdManager->GetElement(i).GetOffsetValue(timestep),"offset" );
      }
    vtkDataArray* d = vtkDataArray::SafeDownCast(a);
    if (d)
      {
      // ranges are only written in case of Data Arrays.
      double *range = d->GetRange(-1);
      this->ForwardAppendedDataDouble
        (pdManager->GetElement(i).GetRangeMinPosition(timestep),
         range[0],"RangeMin" );
      this->ForwardAppendedDataDouble
        (pdManager->GetElement(i).GetRangeMaxPosition(timestep),
         range[1],"RangeMax" );
      }
    a->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCellDataAppended(vtkCellData* cd, vtkIndent indent,
                                         OffsetsManagerGroup *cdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
    }

  os << ">\n";

  cdManager->Allocate(cd->GetNumberOfArrays());
  for(int i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    cdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    for(int t=0; t< this->NumberOfTimeSteps; ++t)
      {
      this->WriteArrayAppended(cd->GetAbstractArray(i), indent.GetNextIndent(),
        cdManager->GetElement(i), names[i], 0, t);
      if (this->ErrorCode != vtkErrorCode::NoError)
        {
        this->DestroyStringArray(cd->GetNumberOfArrays(), names);
        return;
        }
      }
    }

  os << indent << "</CellData>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCellDataAppendedData(vtkCellData* cd, int timestep,
                                             OffsetsManagerGroup *cdManager)
{
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    this->SetProgressRange(progressRange, i, cd->GetNumberOfArrays());
    unsigned long mtime = cd->GetMTime();
    // Only write pd if MTime has changed
    unsigned long &cdMTime = cdManager->GetElement(i).GetLastMTime();
    vtkAbstractArray* a = this->CreateArrayForCells(cd->GetAbstractArray(i));
    if( cdMTime != mtime )
      {
      cdMTime = mtime;
      this->WriteArrayAppendedData
        (a,
         cdManager->GetElement(i).GetPosition(timestep),
         cdManager->GetElement(i).GetOffsetValue(timestep));
      if (this->ErrorCode != vtkErrorCode::NoError)
        {
        return;
        }
      }
    else
      {
      assert( timestep > 0 );
      cdManager->GetElement(i).GetOffsetValue(timestep) =
        cdManager->GetElement(i).GetOffsetValue(timestep-1);
      this->ForwardAppendedDataOffset(
        cdManager->GetElement(i).GetPosition(timestep),
        cdManager->GetElement(i).GetOffsetValue(timestep),
        "offset" );
      }
    vtkDataArray* d = vtkDataArray::SafeDownCast(a);
    if (d)
      {
      double *range = d->GetRange(-1);
      this->ForwardAppendedDataDouble
        (cdManager->GetElement(i).GetRangeMinPosition(timestep),
         range[0],"RangeMin" );
      this->ForwardAppendedDataDouble
        (cdManager->GetElement(i).GetRangeMaxPosition(timestep),
         range[1],"RangeMax" );
      }
    a->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteAttributeIndices(vtkDataSetAttributes* dsa,
                                         char** names)
{
  int i;
  int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);
  for(i=0;i < vtkDataSetAttributes::NUM_ATTRIBUTES;++i)
    {
    if(attributeIndices[i] >= 0)
      {
      const char* attrName = dsa->GetAttributeTypeAsString(i);
      vtkDataArray* a = dsa->GetArray(attributeIndices[i]);
      const char* arrayName = a->GetName();
      if(!arrayName)
        {
        // Assign a name to the array.
        names[attributeIndices[i]] = new char[strlen(attrName)+2];
        strcpy(names[attributeIndices[i]], attrName);
        strcat(names[attributeIndices[i]], "_");
        arrayName = names[attributeIndices[i]];
        }
      this->WriteStringAttribute(attrName, arrayName);
      if (this->ErrorCode != vtkErrorCode::NoError)
        {
        return;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointsAppended(vtkPoints* points, vtkIndent indent,
                                       OffsetsManager *ptManager)
{
  ostream& os = *(this->Stream);

  // Only write points if they exist.
  os << indent << "<Points>\n";
  if(points)
    {
    for(int t=0; t< this->NumberOfTimeSteps; ++t)
      {
      this->WriteArrayAppended(points->GetData(),
        indent.GetNextIndent(), *ptManager, 0, 0, t);
      }
    }
  os << indent << "</Points>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointsAppendedData(vtkPoints* points, int timestep,
                                           OffsetsManager *ptManager)
{
  // Only write points if they exist.
  if(points)
    {
    unsigned long mtime = points->GetMTime();
    // Only write points if MTime has changed
    unsigned long &pointsMTime = ptManager->GetLastMTime();
    // since points->Data is a vtkDataArray.
    vtkDataArray* outPoints = vtkDataArray::SafeDownCast(
      this->CreateArrayForPoints(points->GetData()));
    if( pointsMTime != mtime || timestep == 0 )
      {
      pointsMTime = mtime;
      this->WriteArrayAppendedData(outPoints,
        ptManager->GetPosition(timestep), ptManager->GetOffsetValue(timestep));
      }
    else
      {
      assert( timestep > 0 );
      ptManager->GetOffsetValue(timestep) = ptManager->GetOffsetValue(timestep-1);
      this->ForwardAppendedDataOffset(
        ptManager->GetPosition(timestep),
        ptManager->GetOffsetValue(timestep), "offset");
      }
    double *range = outPoints->GetRange(-1);
    this->ForwardAppendedDataDouble
      (ptManager->GetRangeMinPosition(timestep),
       range[0],"RangeMin" );
    this->ForwardAppendedDataDouble
      (ptManager->GetRangeMaxPosition(timestep),
       range[1],"RangeMax" );
    outPoints->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointsInline(vtkPoints* points, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<Points>\n";
  if(points)
    {
    vtkAbstractArray* outPoints = this->CreateArrayForPoints(points->GetData());
    this->WriteArrayInline(outPoints, indent.GetNextIndent());
    outPoints->Delete();
    }
  os << indent << "</Points>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCoordinatesInline(vtkDataArray* xc, vtkDataArray* yc,
                                          vtkDataArray* zc, vtkIndent indent)
{
  ostream& os = *(this->Stream);

  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  if(xc && yc && zc)
    {
    vtkDataArray* oxc = this->CreateExactCoordinates(xc, 0);
    vtkDataArray* oyc = this->CreateExactCoordinates(yc, 1);
    vtkDataArray* ozc = this->CreateExactCoordinates(zc, 2);

    // Split progress over the three coordinates arrays.
    vtkIdType total = (oxc->GetNumberOfTuples()+
                       oyc->GetNumberOfTuples()+
                       ozc->GetNumberOfTuples());
    if(total == 0)
      {
      total = 1;
      }
    float fractions[4] =
      {
        0,
        float(oxc->GetNumberOfTuples()) / total,
        float(oxc->GetNumberOfTuples()+oyc->GetNumberOfTuples()) / total,
        1
      };
    float progressRange[2] = {0,0};
    this->GetProgressRange(progressRange);

    this->SetProgressRange(progressRange, 0, fractions);
    this->WriteArrayInline(oxc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      oxc->Delete();
      oyc->Delete();
      ozc->Delete();
      return;
      }

    this->SetProgressRange(progressRange, 1, fractions);
    this->WriteArrayInline(oyc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      oxc->Delete();
      oyc->Delete();
      ozc->Delete();
      return;
      }

    this->SetProgressRange(progressRange, 2, fractions);
    this->WriteArrayInline(ozc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      oxc->Delete();
      oyc->Delete();
      ozc->Delete();
      return;
      }

    oxc->Delete();
    oyc->Delete();
    ozc->Delete();
    }
  os << indent << "</Coordinates>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return;
    }
}

//----------------------------------------------------------------------------
void
vtkXMLWriter::WriteCoordinatesAppended(vtkDataArray* xc, vtkDataArray* yc,
                                       vtkDataArray* zc, vtkIndent indent,
                                       OffsetsManagerGroup *coordManager)
{
  ostream& os = *(this->Stream);

  // Helper for the 'for' loop
  vtkDataArray *allcoords[3];
  allcoords[0] = xc;
  allcoords[1] = yc;
  allcoords[2] = zc;

  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  coordManager->Allocate(3);
  if(xc && yc && zc)
    {
    for(int i=0; i<3; ++i)
      {
      coordManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
      for(int t=0; t<this->NumberOfTimeSteps; ++t)
        {
        this->WriteArrayAppended(allcoords[i], indent.GetNextIndent(),
          coordManager->GetElement(i), 0, 0, t);
        if (this->ErrorCode != vtkErrorCode::NoError)
          {
          return ;
          }
        }
      }
    }
  os << indent << "</Coordinates>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCoordinatesAppendedData(vtkDataArray* xc, vtkDataArray* yc,
                                                vtkDataArray* zc, int timestep,
                                                OffsetsManagerGroup *coordManager)
{
  // Only write coordinates if they exist.
  if(xc && yc && zc)
    {
    vtkDataArray* oxc = this->CreateExactCoordinates(xc, 0);
    vtkDataArray* oyc = this->CreateExactCoordinates(yc, 1);
    vtkDataArray* ozc = this->CreateExactCoordinates(zc, 2);

    // Split progress over the three coordinates arrays.
    vtkIdType total = (oxc->GetNumberOfTuples()+
                       oyc->GetNumberOfTuples()+
                       ozc->GetNumberOfTuples());
    if(total == 0)
      {
      total = 1;
      }
    float fractions[4] =
      {
        0,
        float(oxc->GetNumberOfTuples()) / total,
        float(oxc->GetNumberOfTuples()+oyc->GetNumberOfTuples()) / total,
        1
      };
    float progressRange[2] = {0,0};
    this->GetProgressRange(progressRange);

    // Helper for the 'for' loop
    vtkDataArray *allcoords[3];
    allcoords[0] = oxc;
    allcoords[1] = oyc;
    allcoords[2] = ozc;

    for(int i=0; i<3; ++i)
      {
      this->SetProgressRange(progressRange, i, fractions);
      unsigned long mtime = allcoords[i]->GetMTime();
      // Only write pd if MTime has changed
      unsigned long &coordMTime = coordManager->GetElement(i).GetLastMTime();
      if( coordMTime != mtime )
        {
        coordMTime = mtime;
        this->WriteArrayAppendedData(allcoords[i],
          coordManager->GetElement(i).GetPosition(timestep),
          coordManager->GetElement(i).GetOffsetValue(timestep));
        if (this->ErrorCode != vtkErrorCode::NoError)
          {
          oxc->Delete();
          oyc->Delete();
          ozc->Delete();
          return;
          }
        }
      else
        {
        }
      }

    oxc->Delete();
    oyc->Delete();
    ozc->Delete();
    }
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkXMLWriter::CreateArrayForPoints(vtkAbstractArray* inArray)
{
  // Only some subclasses need to do anything.  By default, just
  // return the array as given.
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkXMLWriter::CreateArrayForCells(vtkAbstractArray* inArray)
{
  // Only some subclasses need to do anything.  By default, just
  // return the array as given.
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkXMLWriter::CreateExactCoordinates(vtkDataArray* inArray, int)
{
  // This method is just a dummy because we don't want a pure virtual.
  // Subclasses that need it should define the real version.
  vtkErrorMacro("vtkXMLWriter::CreateExactCoordinates should never be called.");
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePPointData(vtkPointData* pd, vtkIndent indent)
{
  if(pd->GetNumberOfArrays() == 0)
    {
    return;
    }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PPointData";
  this->WriteAttributeIndices(pd, names);
  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
    }
  os << ">\n";

  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    this->WritePArray(pd->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      this->DestroyStringArray(pd->GetNumberOfArrays(), names);
      return;
      }
    }

  os << indent << "</PPointData>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePCellData(vtkCellData* cd, vtkIndent indent)
{
  if(cd->GetNumberOfArrays() == 0)
    {
    return;
    }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<PCellData";
  this->WriteAttributeIndices(cd, names);
  os << ">\n";

  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    this->WritePArray(cd->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
    }

  os << indent << "</PCellData>\n";

  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePPoints(vtkPoints* points, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<PPoints>\n";
  if(points)
    {
    this->WritePArray(points->GetData(), indent.GetNextIndent());
    }
  os << indent << "</PPoints>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePArray(vtkAbstractArray* a, vtkIndent indent,
                                   const char* alternateName)
{
  vtkDataArray* d = vtkDataArray::SafeDownCast(a);
  ostream& os = *(this->Stream);
  if (d)
    {
    os << indent << "<PDataArray";
    }
  else
    {
    os << indent << "<PArray";
    }
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if(alternateName)
    {
    this->WriteStringAttribute("Name", alternateName);
    }
  else
    {
    const char* arrayName = a->GetName();
    if(arrayName)
      {
      this->WriteStringAttribute("Name", arrayName);
      }
    }
  if(a->GetNumberOfComponents() > 1)
    {
    this->WriteScalarAttribute("NumberOfComponents",
                               a->GetNumberOfComponents());
    }
  os << "/>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePCoordinates(vtkDataArray* xc, vtkDataArray* yc,
                                     vtkDataArray* zc, vtkIndent indent)
{
  ostream& os = *(this->Stream);

  // Only write coordinates if they exist.
  os << indent << "<PCoordinates>\n";
  if(xc && yc && zc)
    {
    this->WritePArray(xc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    this->WritePArray(yc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    this->WritePArray(zc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    }
  os << indent << "</PCoordinates>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
char** vtkXMLWriter::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  int i;
  for(i=0; i < numStrings; ++i)
    {
    strings[i] = 0;
    }
  return strings;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::DestroyStringArray(int numStrings, char** strings)
{
  int i;
  for(i=0; i < numStrings; ++i)
    {
    if(strings[i])
      {
      delete [] strings[i];
      }
    }
  delete [] strings;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::GetProgressRange(float* range)
{
  range[0] = this->ProgressRange[0];
  range[1] = this->ProgressRange[1];
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetProgressRange(float* range, int curStep, int numSteps)
{
  float stepSize = (range[1] - range[0])/numSteps;
  this->ProgressRange[0] = range[0] + stepSize*curStep;
  this->ProgressRange[1] = range[0] + stepSize*(curStep+1);
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetProgressRange(float* range, int curStep,
                                    float* fractions)
{
  float width = range[1] - range[0];
  this->ProgressRange[0] = range[0] + fractions[curStep]*width;
  this->ProgressRange[1] = range[0] + fractions[curStep+1]*width;
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetProgressPartial(float fraction)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  this->UpdateProgressDiscrete(this->ProgressRange[0] + fraction*width);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::UpdateProgressDiscrete(float progress)
{
  if(!this->AbortExecute)
    {
    // Round progress to nearest 100th.
    float rounded = float(int((progress*100)+0.5))/100;
    if(this->GetProgress() != rounded)
      {
      this->UpdateProgress(rounded);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePrimaryElementAttributes(ostream &os, vtkIndent indent)
{
  // Write the time step if any:
  if( this->NumberOfTimeSteps > 1)
    {
    // First thing allocate NumberOfTimeValues
    assert( this->NumberOfTimeValues == NULL );
    this->NumberOfTimeValues = new vtkTypeInt64[this->NumberOfTimeSteps];
    os << indent << "TimeValues=\"\n";

    std::string blankline = std::string(40, ' '); //enough room for precision
    for(int i=0; i<this->NumberOfTimeSteps; i++)
      {
      this->NumberOfTimeValues[i] = os.tellp();
      os << blankline.c_str() << "\n";
      }
    os << "\"";
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WritePrimaryElement(ostream &os, vtkIndent indent)
{
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName();

  this->WritePrimaryElementAttributes(os,indent);

  // Close the primary element:
  os << ">\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
    }
  return 1;
}

// The following function are designed to be called outside of the VTK pipeline
// typically from a C interface or when ParaView want to control the writing
//----------------------------------------------------------------------------
void vtkXMLWriter::Start()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return;
    }
  this->UserContinueExecuting = 1;
}

//----------------------------------------------------------------------------
// The function does not make sense in the general case but we need to handle
// the case where the simulation stop before reaching the number of steps
// specified by the user. Therefore the CurrentTimeIndex is never equal
// to NumberOfTimeStep and thus we need to force closing of the xml file
void vtkXMLWriter::Stop()
{
  this->UserContinueExecuting = 0;
  this->Modified();
  this->Update();
  this->UserContinueExecuting = -1; //put back the writer in initial state
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteNextTime(double time)
{
  this->Modified();
  this->Update();

  ostream& os = *(this->Stream);

  if (this->NumberOfTimeValues)
    {
    // Write user specified time value in the TimeValues attribute
    std::streampos returnPos = os.tellp();
    vtkTypeInt64 t = this->NumberOfTimeValues[this->CurrentTimeIndex-1];
    os.seekp(std::streampos(t));
    os << time;
    os.seekp(returnPos);
    }
}


