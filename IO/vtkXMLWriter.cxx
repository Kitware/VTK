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
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkZLibDataCompressor.h"
#define vtkOffsetsManager_DoNotInclude
#include "vtkOffsetsManagerArray.h"
#undef  vtkOffsetsManager_DoNotInclude

#include <assert.h>
#include <vtkstd/string>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkXMLWriter, "1.58.2.1");
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
void vtkXMLWriter::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->SetInputConnection(index, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
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
void vtkXMLWriter::SetBlockSize(unsigned int blockSize)
{
  // Enforce constraints on block size.
  unsigned int nbs = blockSize;
#if VTK_SIZEOF_DOUBLE > VTK_SIZEOF_ID_TYPE
  typedef double LargestScalarType;
#else
  typedef vtkIdType LargestScalarType;
#endif
  unsigned int remainder = nbs % sizeof(LargestScalarType);
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
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::GetDataSetMinorVersion()
{
  return 1;
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
vtkXMLWriter::OffsetType
vtkXMLWriter::ReserveAttributeSpace(const char* attr, int length)
{
  // Save the starting stream position.
  ostream& os = *(this->Stream);
  OffsetType startPosition = os.tellp();

  // By default write an empty valid xml: attr="".  In most case it
  // will be overwritten but we guarantee that the xml produced will
  // be valid in case we stop writting too early.
  os << " " << attr << "=\"\"";

  // Now reserve space for the value.
  for(int i=0; i < length; ++i)
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
unsigned long vtkXMLWriter::GetAppendedDataOffset()
{
  unsigned long pos = this->Stream->tellp();
  return (pos - this->AppendedDataPosition);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::WriteAppendedDataOffset(unsigned long streamPos,
                                                    unsigned long &lastoffset,
                                                    const char* attr)
{
  // Write an XML attribute with the given name.  The value is the
  // current appended data offset.  Starts writing at the given stream
  // position, and returns the ending position.  If attr is 0, writes
  // only the double quotes.  In all cases, the final stream position
  // is left the same as before the call.
  ostream& os = *(this->Stream);
  unsigned long returnPos = os.tellp();
  unsigned long offset = returnPos - this->AppendedDataPosition;
  lastoffset = offset; //saving result
  os.seekp(streamPos);
  if(attr)
    {
    os << " " << attr << "=";
    }
  os << "\"" << offset << "\"";
  unsigned long endPos = os.tellp();
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  return endPos;
}
//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::ForwardAppendedDataOffset(unsigned long streamPos,
                                                      unsigned long offset,
                                                      const char* attr)
{
  ostream& os = *(this->Stream);
  unsigned long returnPos = os.tellp();
  os.seekp(streamPos);
  if(attr)
    {
    os << " " << attr << "=";
    }
  os << "\"" << offset << "\"";
  unsigned long endPos = os.tellp();
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  return endPos;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryData(void* data, int numWords, int wordType)
{
  unsigned long outWordSize = this->GetOutputWordTypeSize(wordType);
  if(this->Compressor)
    {
    // Need to compress the data.  Create compression header.  This
    // reserves enough space in the output.
    if(!this->CreateCompressionHeader(numWords*outWordSize))
      {
      return 0;
      }

    // Start writing the data.
    int result = this->DataStream->StartWriting();

    // Process the actual data.
    if(result && !this->WriteBinaryDataInternal(data, numWords, wordType))
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
      delete [] this->CompressionHeader;
      this->CompressionHeader = 0;
      }

    return result;
    }
  else
    {
    // No data compression.  The header is just the length of the data.
    HeaderType length = numWords*outWordSize;
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    this->PerformByteSwap(p, 1, sizeof(HeaderType));

    // Start writing the data.
    if(!this->DataStream->StartWriting())
      {
      return 0;
      }

    // Write the header consisting only of the data length.
    int writeRes = this->DataStream->Write(p, sizeof(HeaderType));
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
    if(!this->WriteBinaryDataInternal(data, numWords, wordType))
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
int vtkXMLWriter::WriteBinaryDataInternal(void* data, int numWords,
                                          int wordType)
{
  // Break into blocks and handle each one separately.  This allows
  // for better random access when reading compressed data and saves
  // memory during writing.

  // The size of the blocks written (before compression) is
  // this->BlockSize.  We need to support the possibility that the
  // size of data in memory and the size on disk are different.  This
  // is necessary to allow vtkIdType to be converted to UInt32 for
  // writing.
  unsigned long memWordSize = this->GetWordTypeSize(wordType);
  unsigned long outWordSize = this->GetOutputWordTypeSize(wordType);
  unsigned long blockWords = this->BlockSize/outWordSize;
  unsigned long memBlockSize = blockWords*memWordSize;

#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    this->Int32IdTypeBuffer = new Int32IdType[blockWords];
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
      this->ByteSwapBuffer = new unsigned char[blockWords*outWordSize];
      }
    }

  // Prepare a pointer and counter to move through the data.
  unsigned char* ptr = reinterpret_cast<unsigned char*>(data);
  unsigned long wordsLeft = numWords;

  // Do the complete blocks.
  this->SetProgressPartial(0);
  int result = 1;
  while(result && (wordsLeft >= blockWords))
    {
    if(!this->WriteBinaryDataBlock(ptr, blockWords, wordType))
      {
      result = 0;
      }
    ptr += memBlockSize;
    wordsLeft -= blockWords;
    this->SetProgressPartial(float(numWords-wordsLeft)/numWords);
    }

  // Do the last partial block if any.
  if(result && (wordsLeft > 0))
    {
    if(!this->WriteBinaryDataBlock(ptr, wordsLeft, wordType))
      {
      result = 0;
      }
    }
  this->SetProgressPartial(1);

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

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryDataBlock(unsigned char* in_data, int numWords,
                                       int wordType)
{
  unsigned char* data = in_data;
#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    vtkIdType* idBuffer = reinterpret_cast<vtkIdType*>(in_data);

    int i;
    for(i=0;i < numWords; ++i)
      {
      this->Int32IdTypeBuffer[i] = static_cast<Int32IdType>(idBuffer[i]);
      }

    data = reinterpret_cast<unsigned char*>(this->Int32IdTypeBuffer);
    }
#endif

  // Get the word size of the data buffer.  This is now the size that
  // will be written.
  unsigned long wordSize = this->GetOutputWordTypeSize(wordType);

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
void vtkXMLWriter::PerformByteSwap(void* data, int numWords, int wordSize)
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
int vtkXMLWriter::CreateCompressionHeader(unsigned long size)
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
  unsigned long numFullBlocks = size / this->BlockSize;
  unsigned long lastBlockSize = size % this->BlockSize;
  unsigned int numBlocks = numFullBlocks + (lastBlockSize?1:0);

  unsigned int headerLength = numBlocks+3;
  this->CompressionHeaderLength = headerLength;

  this->CompressionHeader = new HeaderType[headerLength];

  // Write out dummy header data.
  unsigned int i;
  for(i=0; i < headerLength; ++i) { this->CompressionHeader[i] = 0; }

  this->CompressionHeaderPosition = this->Stream->tellp();
  unsigned char* ch =
    reinterpret_cast<unsigned char*>(this->CompressionHeader);
  unsigned int chSize = (this->CompressionHeaderLength*sizeof(HeaderType));

  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(ch, chSize) &&
                this->DataStream->EndWriting());

  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  // Fill in known header data now.
  this->CompressionHeader[0] = numBlocks;
  this->CompressionHeader[1] = this->BlockSize;
  this->CompressionHeader[2] = lastBlockSize;

  // Initialize counter for block writing.
  this->CompressionBlockNumber = 0;

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionBlock(unsigned char* data,
                                        unsigned long size)
{
  // Compress the data.
  vtkUnsignedCharArray* outputArray = this->Compressor->Compress(data, size);

  // Find the compressed size.
  HeaderType outputSize = outputArray->GetNumberOfTuples();
  unsigned char* outputPointer = outputArray->GetPointer(0);

  // Write the compressed data.
  int result = this->DataStream->Write(outputPointer, outputSize);
  this->Stream->flush();
  if (this->Stream->fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }

  // Store the resulting compressed size in the compression header.
  this->CompressionHeader[3+this->CompressionBlockNumber++] = outputSize;

  outputArray->Delete();

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionHeader()
{
  // Write real compression header back into stream.
  unsigned long returnPosition = this->Stream->tellp();

  // Need to byte-swap header.
  this->PerformByteSwap(this->CompressionHeader, this->CompressionHeaderLength,
                        sizeof(HeaderType));

  if(!this->Stream->seekp(this->CompressionHeaderPosition)) { return 0; }
  unsigned char* ch =
    reinterpret_cast<unsigned char*>(this->CompressionHeader);
  unsigned int chSize = (this->CompressionHeaderLength*sizeof(HeaderType));
  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(ch, chSize) &&
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
unsigned long vtkXMLWriter::GetOutputWordTypeSize(int dataType)
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
unsigned long vtkXMLWriterGetWordTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::GetWordTypeSize(int dataType)
{
  unsigned long size = 1;
  switch (dataType)
    {
    vtkTemplateMacro(
      size = vtkXMLWriterGetWordTypeSize(static_cast<VTK_TT*>(0))
      );
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
template <class T>
int vtkXMLWriteAsciiData(ostream& os, T* data, int length, vtkIndent indent,
                         long)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  for(r=0;r < rows;++r)
    {
    os << indent << data[pos++];
    for(c=1;c < columns;++c)
      {
      os << " " << data[pos++];
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << data[pos++];
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << data[pos++];
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriteAsciiData(ostream& os, char* data, int length, vtkIndent indent,
                         int)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  for(r=0;r < rows;++r)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < columns;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriteAsciiData(ostream& os, unsigned char* data, int length,
                         vtkIndent indent, int)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  typedef unsigned short ushort;
  for(r=0;r < rows;++r)
    {
    os << indent << ushort(data[pos++]);
    for(c=1;c < columns;++c)
      {
      os << " " << ushort(data[pos++]);
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << ushort(data[pos++]);
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << ushort(data[pos++]);
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriteAsciiData(ostream& os, signed char* data, int length,
                         vtkIndent indent, int)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  for(r=0;r < rows;++r)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < columns;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteAsciiData(void* data, int numWords, int wordType,
                                 vtkIndent indent)
{
  void* b = data;
  int nw = numWords;
  vtkIndent i = indent;
  this->Stream->precision(11);
  ostream& os = *(this->Stream);
  switch(wordType)
    {
    vtkTemplateMacro(
      return vtkXMLWriteAsciiData(os, static_cast<VTK_TT*>(b), nw, i, 1)
      );
    default:
      return 0;
    }
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::WriteDataArrayAppended(vtkDataArray* a,
                                                   vtkIndent indent,
                                                   const char* alternateName,
                                                   int writeNumTuples,
                                                   int timestep)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray:
  this->WriteDataArrayHeader(a,indent,alternateName, writeNumTuples, timestep);
  unsigned long pos = this->ReserveAttributeSpace("offset");
  // Close the header
  os << "/>\n";
  this->WriteDataArrayFooter(os, indent);

  return pos;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteDataArrayAppendedData(vtkDataArray* a,
                                              unsigned long pos,
                                              unsigned long &lastoffset)
{
  this->WriteAppendedDataOffset(pos, lastoffset, "offset");
  if (this->ErrorCode != vtkErrorCode::NoError)
    {
    return;
    }
  this->WriteBinaryData(a->GetVoidPointer(0),
                        a->GetNumberOfTuples()*a->GetNumberOfComponents(),
                        a->GetDataType());
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteDataArrayHeader(vtkDataArray* a, vtkIndent indent,
                                        const char* alternateName,
                                        int writeNumTuples,
                                        int timestep)
{
  ostream& os = *(this->Stream);
  os << indent << "<DataArray";
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
void vtkXMLWriter::WriteDataArrayFooter(ostream &os, vtkIndent )
{
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}
//----------------------------------------------------------------------------
void vtkXMLWriter::WriteDataArrayInline(vtkDataArray* a, vtkIndent indent,
                                        const char* alternateName,
                                        int writeNumTuples)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray:
  this->WriteDataArrayHeader(a, indent, alternateName, writeNumTuples, -1);
  // Close the header
  os << ">\n";
  // Write the data
  this->WriteInlineData(a->GetVoidPointer(0),
                        a->GetNumberOfTuples()*a->GetNumberOfComponents(),
                        a->GetDataType(), indent.GetNextIndent());
  // Close the </DataArray>
  os << indent << "</DataArray>\n";
  this->WriteDataArrayFooter(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteInlineData(void* data, int numWords, int wordType,
                                   vtkIndent indent)
{
  if(this->DataMode == vtkXMLWriter::Binary)
    {
    ostream& os = *(this->Stream);
    os << indent;
    this->WriteBinaryData(data, numWords, wordType);
    os << "\n";
    }
  else
    {
    this->WriteAsciiData(data, numWords, wordType, indent);
    }
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
    this->WriteDataArrayInline(fd->GetArray(i), indent.GetNextIndent(),
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
    vtkDataArray* a = this->CreateArrayForPoints(pd->GetArray(i));
    this->WriteDataArrayInline(a, indent.GetNextIndent(), names[i]);
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
    vtkDataArray* a = this->CreateArrayForCells(cd->GetArray(i));
    this->WriteDataArrayInline(a, indent.GetNextIndent(), names[i]);
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
  fdManager->Allocate(fd->GetNumberOfArrays());
  for(i=0; i < fd->GetNumberOfArrays(); ++i)
    {
    fdManager->GetElement(i).Allocate(1);
    fdManager->GetElement(i).GetPosition(0) = 
      this->WriteDataArrayAppended(fd->GetArray(i),
                                   indent.GetNextIndent(),
                                   names[i], 1 , -1);
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
    this->WriteDataArrayAppendedData(fd->GetArray(i),
      fdManager->GetElement(i).GetPosition(timestep),
      fdManager->GetElement(i).GetOffsetValue(timestep));
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
      pdManager->GetElement(i).GetPosition(t) =
        this->WriteDataArrayAppended(pd->GetArray(i),
                                     indent.GetNextIndent(),
                                     names[i],0,t);
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
    if( pdMTime != mtime )
      {
      pdMTime = mtime;
      vtkDataArray* a = this->CreateArrayForPoints(pd->GetArray(i));
      this->WriteDataArrayAppendedData(a, pdManager->GetElement(i).GetPosition(timestep),
                                          pdManager->GetElement(i).GetOffsetValue(timestep));
      a->Delete();
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
      this->ForwardAppendedDataOffset(pdManager->GetElement(i).GetPosition(timestep),
                                      pdManager->GetElement(i).GetOffsetValue(timestep),
                                      "offset" );
      }
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
      cdManager->GetElement(i).GetPosition(t) = 
        this->WriteDataArrayAppended(cd->GetArray(i),
                                     indent.GetNextIndent(),
                                     names[i],0,t);
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
    if( cdMTime != mtime )
      {
      cdMTime = mtime;
      vtkDataArray* a = this->CreateArrayForCells(cd->GetArray(i));
      this->WriteDataArrayAppendedData(a, cdManager->GetElement(i).GetPosition(timestep),
                                          cdManager->GetElement(i).GetOffsetValue(timestep));
      a->Delete();
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
      ptManager->GetPosition(t) =
        this->WriteDataArrayAppended(points->GetData(), indent.GetNextIndent(),0,0,t);
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
    if( pointsMTime != mtime )
      {
      pointsMTime = mtime;
      vtkDataArray* outPoints = this->CreateArrayForPoints(points->GetData());
      this->WriteDataArrayAppendedData(outPoints, ptManager->GetPosition(timestep),
                                                  ptManager->GetOffsetValue(timestep));
      outPoints->Delete();
      }
    else
      {
      assert( timestep > 0 );
      ptManager->GetOffsetValue(timestep) = ptManager->GetOffsetValue(timestep-1);
      this->ForwardAppendedDataOffset( 
        ptManager->GetPosition(timestep),
        ptManager->GetOffsetValue(timestep), "offset");
      }
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
    vtkDataArray* outPoints = this->CreateArrayForPoints(points->GetData());
    this->WriteDataArrayInline(outPoints, indent.GetNextIndent());
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
    this->WriteDataArrayInline(oxc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      oxc->Delete();
      oyc->Delete();
      ozc->Delete();
      return;
      }

    this->SetProgressRange(progressRange, 1, fractions);
    this->WriteDataArrayInline(oyc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      oxc->Delete();
      oyc->Delete();
      ozc->Delete();
      return;
      }

    this->SetProgressRange(progressRange, 2, fractions);
    this->WriteDataArrayInline(ozc, indent.GetNextIndent());
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
        coordManager->GetElement(i).GetPosition(t) =
          this->WriteDataArrayAppended(allcoords[i], indent.GetNextIndent());
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
        this->WriteDataArrayAppendedData(allcoords[i], 
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
vtkDataArray* vtkXMLWriter::CreateArrayForPoints(vtkDataArray* inArray)
{
  // Only some subclasses need to do anything.  By default, just
  // return the array as given.
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkXMLWriter::CreateArrayForCells(vtkDataArray* inArray)
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
    this->WritePDataArray(pd->GetArray(i), indent.GetNextIndent(), names[i]);
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
    this->WritePDataArray(cd->GetArray(i), indent.GetNextIndent(), names[i]);
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
    this->WritePDataArray(points->GetData(), indent.GetNextIndent());
    }
  os << indent << "</PPoints>\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePDataArray(vtkDataArray* a, vtkIndent indent,
                                   const char* alternateName)
{
  ostream& os = *(this->Stream);
  os << indent << "<PDataArray";
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
    this->WritePDataArray(xc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    this->WritePDataArray(yc, indent.GetNextIndent());
    if (this->ErrorCode != vtkErrorCode::NoError)
      {
      return;
      }
    this->WritePDataArray(zc, indent.GetNextIndent());
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
    this->NumberOfTimeValues = new unsigned long[this->NumberOfTimeSteps];
    os << indent << "TimeValues=\"\n";
    
    vtkstd::string blankline = vtkstd::string(40, ' '); //enough room for precision
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
    unsigned long returnPos = os.tellp();
    os.seekp(this->NumberOfTimeValues[this->CurrentTimeIndex-1]);
    os << time;
    os.seekp(returnPos);
    }
}


