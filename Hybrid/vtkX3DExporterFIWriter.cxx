/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterFIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporterFIWriter.h"

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkX3D.h"

#include <vtksys/ios/sstream>
#include <cassert>
#include <vector>

//#define ENCODEASSTRING 1

using namespace vtkX3D;

/*======================================================================== */
struct NodeInfo 
{
  NodeInfo(int _nodeId)
    {
    this->nodeId = _nodeId;
    this->isChecked = false;
    this->attributesTerminated = true;
    }
  int nodeId;
  bool attributesTerminated;
  bool isChecked;
};

/*======================================================================== */
typedef std::vector<NodeInfo> vtkX3DExporterFINodeInfoStackBase;
class vtkX3DExporterFINodeInfoStack : 
  public vtkX3DExporterFINodeInfoStackBase
{
};

/*======================================================================== */
class vtkX3DExporterFIByteWriter
{
public:
  ~vtkX3DExporterFIByteWriter();
  vtkX3DExporterFIByteWriter() {};
  // This is the current byte to fill
  unsigned char CurrentByte;
  // This is the current byte position. Range: 0-7
  unsigned char CurrentBytePos;

  // Opens the specified file in binary mode. Returns 0
  // if failed
  int OpenFile(const char* file);
  int OpenStream();

  // Puts a bitstring to the current byte bit by bit
  void PutBits(const std::string &bitstring);
  // Puts the integer value to the stream using count bits
  // for encoding
  void PutBits(unsigned int value, unsigned char count);
  // Puts on bit to the current byte true = 1, false = 0
  void PutBit(bool on);
  // Puts whole bytes to the file stream. CurrentBytePos must
  // be 0 for this
  void PutBytes(const char* bytes, size_t length);
  // Fills up the current byte with 0 values
  void FillByte();

  // Get stream info
  const char* GetStringStream(int& size);

private:
  unsigned char Append(unsigned int value, unsigned char count);
  void TryFlush();
  ostream* Stream;

  int WriteToOutputString;

  vtkX3DExporterFIByteWriter(const vtkX3DExporterFIByteWriter&); // Not implemented
  void operator=(const vtkX3DExporterFIByteWriter&); // Not implemented
};

//----------------------------------------------------------------------------
vtkX3DExporterFIByteWriter::~vtkX3DExporterFIByteWriter()
{
  if(this->Stream != NULL)
    {
    delete this->Stream;
    this->Stream = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkX3DExporterFIByteWriter::OpenFile(const char* file)
{
  this->WriteToOutputString = 0;
  this->CurrentByte = 0;
  this->CurrentBytePos = 0;
  ofstream* fileStream = new ofstream();
  fileStream->open (file, ios::out | ios::binary);
  if(fileStream->fail())
    {
    delete fileStream;
    return 0;
    }
  else
    {
    this->Stream = fileStream;
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkX3DExporterFIByteWriter::OpenStream()
{
  this->WriteToOutputString = 1;
  this->CurrentByte = 0;
  this->CurrentBytePos = 0;
  this->Stream = new vtksys_ios::ostringstream();
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkX3DExporterFIByteWriter::GetStringStream(int& size)
{
  if (this->WriteToOutputString && this->Stream)
    {
    vtksys_ios::ostringstream *ostr =
      static_cast<vtksys_ios::ostringstream*>(this->Stream);

    size = static_cast<int>(ostr->str().size());
    return ostr->str().c_str();
    }

  size = 0;
  return NULL;
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::TryFlush()
{
  if (this->CurrentBytePos == 8)
    {
    this->Stream->write((char*)(&(this->CurrentByte)), 1);
    this->CurrentByte = 0;
    this->CurrentBytePos = 0;
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::FillByte()
{
  while (this->CurrentBytePos !=0)
    {
    this->PutBit(0);
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::PutBit(bool on)
{
  assert(this->CurrentBytePos < 8);
  if (on)
    {
    unsigned char pos = this->CurrentBytePos;
    unsigned char mask = (unsigned char)(0x80 >> pos);
    this->CurrentByte |= mask;
    }
  this->CurrentBytePos++;
  TryFlush();
}

//----------------------------------------------------------------------------
unsigned char vtkX3DExporterFIByteWriter::Append(unsigned int value, unsigned char count)
{
  assert(this->CurrentBytePos < 8);
  while ((this->CurrentBytePos < 8) && count > 0)
    {
    // Value and der Stelle i
    unsigned int mask = 1;
    bool isSet = !(((mask << (count - 1)) & value) == 0);
    if (isSet)
      {
      this->CurrentByte |= static_cast<unsigned char>(0x80 >> this->CurrentBytePos);
      }
    this->CurrentBytePos++;
    count--;
    }
  TryFlush();
  return count;
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::PutBytes(const char* bytes, size_t length)
{
  if(this->CurrentBytePos == 0)
    {
    this->Stream->write(bytes, length);
    }
  else
    {
    //vtkErrorMacro(<< "Wrong position in vtkX3DExporterFIByteWriter::PutBytes");
    assert(false);
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::PutBits(unsigned int value, unsigned char count)
{
  // Can be optimized
  while (count > 0)
    {
    count = this->Append(value, count);
    }
}


//----------------------------------------------------------------------------
void vtkX3DExporterFIByteWriter::PutBits(const std::string &bitstring)
{
  std::string::const_iterator I = bitstring.begin();
  while(I != bitstring.end())
    {
    this->PutBit((*I) == '1');
    I++;
    }
}

#include "vtkX3DExporterFIWriterHelper.h"

/* ------------------------------------------------------------------------- */
vtkStandardNewMacro(vtkX3DExporterFIWriter);
//----------------------------------------------------------------------------
vtkX3DExporterFIWriter::~vtkX3DExporterFIWriter()
{
  this->CloseFile();
  delete this->InfoStack;
  this->Compressor->Delete();
}

//----------------------------------------------------------------------------
vtkX3DExporterFIWriter::vtkX3DExporterFIWriter()
{
  this->InfoStack = new vtkX3DExporterFINodeInfoStack();
  this->Compressor = vtkZLibDataCompressor::New();
  this->Compressor->SetCompressionLevel(5);
  this->Writer = NULL;
  this->IsLineFeedEncodingOn = true;
  this->Fastest = 0;
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Fastest: " << this->Fastest << endl;
}

//----------------------------------------------------------------------------
int vtkX3DExporterFIWriter::OpenFile(const char* file)
{
  std::string t(file);
  this->CloseFile();

  // Delegate to vtkX3DExporterFIByteWriter
  this->Writer = new vtkX3DExporterFIByteWriter();
  this->WriteToOutputString = 0;
  return this->Writer->OpenFile(file);
}

int vtkX3DExporterFIWriter::OpenStream()
{
  // Delegate to vtkX3DExporterFIByteWriter
  this->Writer = new vtkX3DExporterFIByteWriter();
  this->WriteToOutputString = 1;
  return this->Writer->OpenStream();
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::CloseFile()
{
  if(this->Writer != NULL)
    {
    if(this->WriteToOutputString)
      {
      if(this->OutputString)
        {
        delete [] this->OutputString;
        this->OutputString = NULL;
        }
      const char* tmp = this->Writer->GetStringStream(this->OutputStringLength);
      this->OutputString = new char[this->OutputStringLength];
      memcpy(this->OutputString, tmp, this->OutputStringLength);
      }
    delete this->Writer;
    this->Writer = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::StartDocument()
{
  const char* external_voc = "urn:external-vocabulary";
  // ITU 12.6: 1110000000000000
  this->Writer->PutBits("1110000000000000");
  // ITU 12.7 / 12.9: Version of standard: 1 as 16bit
  this->Writer->PutBits("0000000000000001");
  // ITU 12.8: The bit '0' (padding) shall then be appended to the bit stream
  this->Writer->PutBit(0);
  // ITU C.2.3 
  this->Writer->PutBit(0); // additional-data
  this->Writer->PutBit(1); // initial-vocabulary
  this->Writer->PutBit(0); // notations
  this->Writer->PutBit(0); // unparsed-entities 
  this->Writer->PutBit(0); // character-encoding-scheme
  this->Writer->PutBit(0); // standalone
  this->Writer->PutBit(0); // and version
  // ITU C.2.5: padding '000' for optional component initial-vocabulary
  this->Writer->PutBits("000");
  // ITU C.2.5.1: For each of the thirteen optional components:
  // presence ? 1 : 0
  this->Writer->PutBits("1000000000000"); // 'external-vocabulary'
  // ITU C.2.5.2: external-vocabulary is present
  this->Writer->PutBit(0); 
  // Write "urn:external-vocabulary"
  // ITU C.22.3.1: Length is < 65
  this->Writer->PutBit(0); 
  //Writer->PutBits("010110"); // = strlen(external_voc) - 1
  this->Writer->PutBits(
    static_cast<unsigned int>(strlen(external_voc) - 1), 6);
  this->Writer->PutBytes(external_voc, strlen(external_voc));
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::EndDocument()
{
  // ITU C.2.12: The four bits '1111' (termination) are appended
  this->Writer->PutBits("1111");
}


//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::StartNode(int elementID)
{
  if (!this->InfoStack->empty())
    {
    this->CheckNode(false);
    if (this->IsLineFeedEncodingOn)
      {
      vtkX3DExporterFIWriterHelper::EncodeLineFeed(this->Writer);
      }
    this->Writer->FillByte();
    }

  this->InfoStack->push_back(NodeInfo(elementID));

  // ITU C.3.7.2: element is present
  this->Writer->PutBit(0);
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::EndNode()
{
  assert(!this->InfoStack->empty());
  this->CheckNode(false);
  if (this->IsLineFeedEncodingOn)
    {
    vtkX3DExporterFIWriterHelper::EncodeLineFeed(this->Writer);
    }
  if(!this->InfoStack->back().attributesTerminated)
    {
    //cout << "Terminated in EndNode: could be wrong" << endl;
    // ITU C.3.6.2: End of attribute
    this->Writer->PutBits("1111");
    }
  // ITU C.3.8: The four bits '1111' (termination) are appended.
  this->Writer->PutBits("1111");
  this->InfoStack->pop_back();
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::CheckNode(bool callerIsAttribute)
{
  if(!this->InfoStack->back().isChecked)
    {
    if (callerIsAttribute) // Element has attributes
      {
      // ITU C.3.3: then the bit '1' (presence) is appended
      this->Writer->PutBit(1);
      this->InfoStack->back().attributesTerminated = false;
      }
    else // Element has no attributes
      {
      // ITU C.3.3: otherwise, the bit '0' (absence) is appended
      this->Writer->PutBit(0);
      }
    // Write Node name (starting at third bit)
    // ITU: C.18.4 If the alternative name-surrogate-index is present, 
    // it is encoded as described in C.27.
    vtkX3DExporterFIWriterHelper::EncodeInteger3(this->Writer, this->InfoStack->back().nodeId + 1);
    this->InfoStack->back().isChecked = true;
    }
  // Element has attributes and childs
  else if (!callerIsAttribute && !this->InfoStack->back().attributesTerminated)
    {
    // ITU C.3.6.2: End of attribute
    this->Writer->PutBits("1111");
    this->InfoStack->back().attributesTerminated = true;
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::StartAttribute(int attributeID, bool literal, bool addToTable)
{
  this->CheckNode();
  // ITU C.3.6.1: Start of attribute
  this->Writer->PutBit(0);
  // ITU C.4.3 The value of qualified-name is encoded as described in C.17.
  vtkX3DExporterFIWriterHelper::EncodeInteger2(this->Writer, attributeID +1);

  // ITU C.14.3: If the alternative literal-character-string is present,
  //then the bit '0' (discriminant) is appended
  // ITU C.14.4: If the alternative string-index is present, 
  // then the bit '1' (discriminant) is appended
  this->Writer->PutBit(literal ? 0 : 1);
  if (literal)
    {
    // ITU C.14.3.1 If the value of the component add-to-table is TRUE, 
    // then the bit '1' is appended to the bit stream;
    this->Writer->PutBit(addToTable);
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::EndAttribute()
{
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, int type, const double* d)
{
  vtksys_ios::ostringstream ss;

  this->StartAttribute(attributeID, true, false);
  
#ifdef ENCODEASSTRING
  const double* loc = NULL;
  size_t size = 0;
  double temp[4];
  switch (type)
    {
    case(SFVEC3F):
    case(SFCOLOR):
      size = 3; loc = d;
      break;
    case(SFROTATION):
      size = 4;
      temp[0] = d[1];
      temp[1] = d[2];
      temp[2] = d[3];
      temp[3] = vtkMath::RadiansFromDegrees( -d[0] );
      loc = temp;
      break;
    default:
      cerr << "UNKNOWN DATATYPE";
      assert(false);
    }
  vtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, loc, size);
#else
  switch (type)
    {
    case(SFVEC3F):
    case(SFCOLOR):
      ss << static_cast<float>( d[0] ) 
         << " " 
         << static_cast<float>( d[1] ) 
         << " " 
         << static_cast<float>( d[2] );
      break;
    case(SFROTATION):
      ss << static_cast<float>( d[1] ) 
         << " " 
         << static_cast<float>( d[2] ) 
         << " " 
         << static_cast<float>( d[3] ) 
         << " " 
         << static_cast<float>( vtkMath::RadiansFromDegrees( -d[0] ) );
      break;
    default:
      cout << "UNKNOWN DATATYPE";
      assert(false);
    }
  vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
#endif
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, int type, vtkDataArray* a)
{
  vtksys_ios::ostringstream ss;
  std::vector<double> values;

  this->StartAttribute(attributeID, true, false);

#ifdef ENCODEASSTRING
  switch(type)
    {
  case (MFVEC3F):
  case (MFVEC2F):
    vtkIdType i;
    for (i = 0; i < a->GetNumberOfTuples(); i++)
      {
      double* d = a->GetTuple(i);
      ss << d[0] << " " << d[1];
      if (type == MFVEC3F) ss << " " << d[2];
      ss << ",";
      }
    vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
    break;
  default:
    cerr << "UNKNOWN DATATYPE";
    assert(false);
    }
#else
  switch(type)
    {
  case (MFVEC3F):
  case (MFVEC2F):
    vtkIdType i;
    for (i = 0; i < a->GetNumberOfTuples(); i++)
      {
      double* d = a->GetTuple(i);
      values.push_back(d[0]);
      values.push_back(d[1]);
      if (type == MFVEC3F)
        {
        values.push_back(d[2]);
        }
      }
    if (!this->Fastest && (values.size() > 15))
      {
      X3DEncoderFunctions::EncodeQuantizedzlibFloatArray(this->Writer, 
        &(values.front()), values.size(), this->Compressor);
      }
    else
      {
      vtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, 
        &(values.front()), values.size());
      }
    break;
  default:
    vtkErrorMacro("UNKNOWN DATATYPE");
    assert(false);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, const double* values,
  size_t size)
{
  this->StartAttribute(attributeID, true, false);
  if (!this->Fastest && (size > 15))
    {
    X3DEncoderFunctions::EncodeQuantizedzlibFloatArray(this->Writer,
      values, size, this->Compressor);
    }
  else
    {
    vtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, values, size);
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID,
  const int* values, size_t size, bool image)
{
  this->StartAttribute(attributeID, true, false);
  if (size > 15)
    {
    X3DEncoderFunctions::EncodeIntegerDeltaZ(this->Writer, values, size, 
      this->Compressor, image);  
    }
  else
    {
    vtkX3DExporterFIWriterHelper::EncodeIntegerFI(this->Writer, values, size);
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, int type, vtkCellArray* a)
{
  vtksys_ios::ostringstream ss;
  std::vector<int> values;

  vtkIdType npts = 0;
  vtkIdType *indx = 0;

  this->StartAttribute(attributeID, true, false);

#ifdef ENCODEASSTRING
  switch(type)
    {
  case (MFINT32):
    int i;
    for (a->InitTraversal(); a->GetNextCell(npts,indx); )
      {
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        ss << (int)indx[i] << " ";
        }
      ss << "-1";
      }
    vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
    break;
  default:
    cerr << "UNKNOWN DATATYPE";
    assert(false);
    }
#else
  switch(type)
    {
  case (MFINT32):
    int i;
    for (a->InitTraversal(); a->GetNextCell(npts,indx); )
      {
      for (i = 0; i < npts; i++)
        {
        values.push_back(indx[i]);
        }
      values.push_back(-1);
      }
    vtkX3DExporterFIWriterHelper::EncodeIntegerFI(this->Writer, &(values.front()), values.size());
    break;
  default:
    cerr << "UNKNOWN DATATYPE";
    assert(false);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, int value)
{
  vtksys_ios::ostringstream ss;
  this->StartAttribute(attributeID, true, false);

  // Xj3D writes out single value fields in string encoding. Expected:
  //FIEncoderFunctions::EncodeFloatFI<float>(this->Writer, &value, 1);
  ss << value;
  vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, float value)
{
  vtksys_ios::ostringstream ss;

  this->StartAttribute(attributeID, true, false);

  // Xj3D writes out single value fields in string encoding. Expected:
  //FIEncoderFunctions::EncodeFloatFI<float>(this->Writer, &value, 1);
  ss << value;
  vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int vtkNotUsed(attributeID), 
  double vtkNotUsed(value))
{
  cout << "Function not implemented yet." << endl;
  assert(false);
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, bool value)
{
  this->StartAttribute(attributeID, false);
  vtkX3DExporterFIWriterHelper::EncodeInteger2(this->Writer, value ? 2 : 1);
}

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::SetField(int attributeID, const char* value,
  bool vtkNotUsed(mfstring))
{
  this->StartAttribute(attributeID, true, true);
  vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, std::string(value));
}

//----------------------------------------------------------------------------
/*void vtkX3DExporterFIWriter::SetField(int attributeID, const std::string &value)
  {
  this->StartAttribute(attributeID, true, true);
  vtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, value);
  }*/

//----------------------------------------------------------------------------
/*void vtkX3DExporterFIWriter::SetField(int attributeID, int type, std::string value)
  {
  assert(type == MFSTRING);
  type++;
  this->SetField(attributeID, value);
  }*/

//----------------------------------------------------------------------------
void vtkX3DExporterFIWriter::Flush()
{

}

