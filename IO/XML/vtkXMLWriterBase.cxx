// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLWriterBase.h"

#include "vtkDataCompressor.h"
#include "vtkLZ4DataCompressor.h"
#include "vtkLZMADataCompressor.h"
#include "vtkObjectFactory.h"
#include "vtkXMLReaderVersion.h"
#include "vtkZLibDataCompressor.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkXMLWriterBase, Compressor, vtkDataCompressor);
//----------------------------------------------------------------------------
vtkXMLWriterBase::vtkXMLWriterBase()
  : FileName(nullptr)
  , WriteToOutputString(false)
#ifdef VTK_WORDS_BIGENDIAN // Byte order defaults to that of machine.
  , ByteOrder(vtkXMLWriterBase::BigEndian)
#else
  , ByteOrder(vtkXMLWriterBase::LittleEndian)
#endif
  , HeaderType(vtkXMLWriterBase::UInt32)
#ifdef VTK_USE_64BIT_IDS // Output vtkIdType size defaults to real size.
  , IdType(vtkXMLWriterBase::Int64)
#else
  , IdType(vtkXMLWriterBase::Int32)
#endif
  , DataMode(vtkXMLWriterBase::Appended)
  , EncodeAppendedData(true)
  , Compressor(vtkZLibDataCompressor::New())
  , BlockSize(32768) // 2^15
  , CompressionLevel(5)
  , UsePreviousVersion(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkXMLWriterBase::~vtkXMLWriterBase()
{
  this->SetFileName(nullptr);
  this->SetCompressor(nullptr);
}

//------------------------------------------------------------------------------
void vtkXMLWriterBase::SetHeaderType(int t)
{
  if (t != vtkXMLWriterBase::UInt32 && t != vtkXMLWriterBase::UInt64)
  {
    vtkErrorMacro(<< this->GetClassName() << " (" << this << "): cannot set HeaderType to " << t);
    return;
  }
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting HeaderType to " << t);
  if (this->HeaderType != t)
  {
    this->HeaderType = t;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkXMLWriterBase::SetIdType(int t)
{
#if !defined(VTK_USE_64BIT_IDS)
  if (t == vtkXMLWriterBase::Int64)
  {
    vtkErrorMacro("Support for Int64 vtkIdType not compiled in VTK.");
    return;
  }
#endif
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting IdType to " << t);
  if (this->IdType != t)
  {
    this->IdType = t;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkXMLWriterBase::SetCompressorType(int compressorType)
{
  if (compressorType == NONE)
  {
    if (this->Compressor)
    {
      this->Compressor->Delete();
      this->Compressor = nullptr;
      this->Modified();
    }
  }
  else if (compressorType == ZLIB)
  {
    if (this->Compressor)
    {
      this->Compressor->Delete();
    }
    this->Compressor = vtkZLibDataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else if (compressorType == LZ4)
  {
    if (this->Compressor)
    {
      this->Compressor->Delete();
    }
    this->Compressor = vtkLZ4DataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else if (compressorType == LZMA)
  {
    if (this->Compressor)
    {
      this->Compressor->Delete();
    }
    this->Compressor = vtkLZMADataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else
  {
    vtkWarningMacro("Invalid compressorType:" << compressorType);
  }
}
//------------------------------------------------------------------------------
void vtkXMLWriterBase::SetCompressionLevel(int compressionLevel)
{
  constexpr int min = 1;
  constexpr int max = 9;
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "CompressionLevel  to " << compressionLevel);
  if (this->CompressionLevel !=
    (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel)))
  {
    this->CompressionLevel =
      (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel));
    if (this->Compressor)
    {
      this->Compressor->SetCompressionLevel(compressionLevel);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkXMLWriterBase::SetBlockSize(size_t blockSize)
{
  // Enforce constraints on block size.
  size_t nbs = blockSize;
#if VTK_SIZEOF_DOUBLE > VTK_SIZEOF_ID_TYPE
  typedef double LargestScalarType;
#else
  typedef vtkIdType LargestScalarType;
#endif
  size_t remainder = nbs % sizeof(LargestScalarType);
  if (remainder)
  {
    nbs -= remainder;
    if (nbs < sizeof(LargestScalarType))
    {
      nbs = sizeof(LargestScalarType);
    }
    vtkWarningMacro("BlockSize must be a multiple of "
      << int(sizeof(LargestScalarType)) << ".  Using " << nbs << " instead of " << blockSize
      << ".");
  }
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting BlockSize to " << nbs);
  if (this->BlockSize != nbs)
  {
    this->BlockSize = nbs;
    this->Modified();
  }
}
//------------------------------------------------------------------------------
int vtkXMLWriterBase::Write()
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

//------------------------------------------------------------------------------
int vtkXMLWriterBase::GetDataSetMajorVersion()
{
  if (this->UsePreviousVersion)
  {
    return (this->HeaderType == vtkXMLWriterBase::UInt64) ? 1 : 0;
  }
  else
  {
    return vtkXMLReaderMajorVersion;
  }
}

//------------------------------------------------------------------------------
int vtkXMLWriterBase::GetDataSetMinorVersion()
{
  if (this->UsePreviousVersion)
  {
    return (this->HeaderType == vtkXMLWriterBase::UInt64) ? 0 : 1;
  }
  else
  {
    return vtkXMLReaderMinorVersion;
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  if (this->ByteOrder == vtkXMLWriterBase::BigEndian)
  {
    os << indent << "ByteOrder: BigEndian\n";
  }
  else
  {
    os << indent << "ByteOrder: LittleEndian\n";
  }
  if (this->IdType == vtkXMLWriterBase::Int32)
  {
    os << indent << "IdType: Int32\n";
  }
  else
  {
    os << indent << "IdType: Int64\n";
  }
  if (this->DataMode == vtkXMLWriterBase::Ascii)
  {
    os << indent << "DataMode: Ascii\n";
  }
  else if (this->DataMode == vtkXMLWriterBase::Binary)
  {
    os << indent << "DataMode: Binary\n";
  }
  else
  {
    os << indent << "DataMode: Appended\n";
  }
  if (this->Compressor)
  {
    os << indent << "Compressor: " << this->Compressor << "\n";
  }
  else
  {
    os << indent << "Compressor: (none)\n";
  }
  os << indent << "EncodeAppendedData: " << this->EncodeAppendedData << "\n";
  os << indent << "BlockSize: " << this->BlockSize << "\n";
}
VTK_ABI_NAMESPACE_END
