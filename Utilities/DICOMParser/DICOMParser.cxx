// SPDX-FileCopyrightText: Copyright (c) 2003 Matt Turek
// SPDX-License-Identifier: BSD-4-Clause
#ifdef _MSC_VER
#pragma warning(disable : 4514)
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#pragma warning(disable : 4710)
#pragma warning(push, 3)
#endif

#include "DICOMParser.h"
#include "DICOMCallback.h"
#include "DICOMConfig.h"

#include <cassert>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include <iostream>
#include <string.h>
#include <string>

#define DICOMPARSER_IGNORE_MAGIC_NUMBER

VTK_ABI_NAMESPACE_BEGIN

static const char* DICOM_MAGIC = "DICM";
static const int OPTIONAL_SKIP = 128;

class DICOMParserImplementation
{
public:
  DICOMParserImplementation() = default;

  std::vector<doublebyte> Groups;
  std::vector<doublebyte> Elements;
  std::vector<DICOMParser::VRTypes> Datatypes;
  //
  // Stores a map from pair<group, element> keys to
  // values of pair<vector<DICOMCallback*>, datatype>
  //
  DICOMParserMap Map;

  //
  // Stores a map from pair<group, element> keys to
  // values of datatype.  We use this to store the
  // datatypes for implicit keys that we are
  // interested in.
  //
  DICOMImplicitTypeMap TypeMap;
};

DICOMParser::DICOMParser()
{
  this->Implementation = new DICOMParserImplementation();
  this->DataFile = nullptr;
  this->ToggleByteSwapImageData = false;
  this->TransferSyntaxCB = new DICOMMemberCallback<DICOMParser>;
  this->InitTypeMap();
  this->FileName = "";
}

const std::string& DICOMParser::GetFileName()
{
  return this->FileName;
}

bool DICOMParser::OpenFile(const std::string& filename)
{
  // Deleting the DataFile closes any previously opened file
  delete this->DataFile;
  this->DataFile = new DICOMFile();
  bool val = this->DataFile->Open(filename);

  if (val)
  {
    this->FileName = filename;
  }

  return val;
}

void DICOMParser::CloseFile()
{
  // Deleting the DataFile closes any previously opened file
  delete this->DataFile;
  this->DataFile = nullptr;
}

DICOMParser::~DICOMParser()
{
  //
  // Delete the callbacks.
  //
  this->ClearAllDICOMTagCallbacks();

  delete this->DataFile;
  delete this->TransferSyntaxCB;
  delete this->Implementation;
}

bool DICOMParser::ReadHeader()
{
  bool dicom = DICOMParser::IsDICOMFile(this->DataFile);
  if (!dicom)
  {
    return false;
  }

  this->TransferSyntaxCB->SetCallbackFunction(this, &DICOMParser::TransferSyntaxCallback);
  this->AddDICOMTagCallback(0x0002, 0x0010, DICOMParser::VR_UI, this->TransferSyntaxCB);

  this->ToggleByteSwapImageData = false;

  doublebyte group = 0;
  doublebyte element = 0;
  DICOMParser::VRTypes datatype = DICOMParser::VR_UNKNOWN;

  this->Implementation->Groups.clear();
  this->Implementation->Elements.clear();
  this->Implementation->Datatypes.clear();

  long fileSize = DataFile->GetSize();
  do
  {
    this->ReadNextRecord(group, element, datatype);

    this->Implementation->Groups.push_back(group);
    this->Implementation->Elements.push_back(element);
    this->Implementation->Datatypes.push_back(datatype);

  } while ((DataFile->Tell() >= 0) && (DataFile->Tell() < fileSize));

  return true;
}

//
// read magic number from file
// return true if this is your image type, false if it is not
//
bool DICOMParser::IsDICOMFile(DICOMFile* file)
{
  char magic_number[4];
  file->SkipToStart();
  file->Read(static_cast<void*>(magic_number), 4);
  if (CheckMagic(magic_number))
  {
    return (true);
  }
  // try with optional skip
  else
  {
    file->Skip(OPTIONAL_SKIP - 4);
    file->Read(static_cast<void*>(magic_number), 4);
    if (CheckMagic(magic_number))
    {
      return true;
    }
    else
    {

#ifndef DICOMPARSER_IGNORE_MAGIC_NUMBER
      return false;
#else
      //
      // Try it anyways...
      //

      file->SkipToStart();

      doublebyte group = file->ReadDoubleByte();
      bool dicom;
      if (group == 0x0002 || group == 0x0008)
      {
        std::cerr << "No DICOM magic number found, but file appears to be DICOM." << std::endl;
        std::cerr << "Proceeding without caution." << std::endl;
        dicom = true;
      }
      else
      {
        dicom = false;
      }
      file->SkipToStart();

      return dicom;
#endif // DICOMPARSER_IGNORE_MAGIC_NUMBER
    }
  }
}

bool DICOMParser::IsValidRepresentation(doublebyte rep, quadbyte& len, VRTypes& mytype)
{
  switch (rep)
  {
    case DICOMParser::VR_AW:
    case DICOMParser::VR_AE:
    case DICOMParser::VR_AS:
    case DICOMParser::VR_CS:
    case DICOMParser::VR_UI:
    case DICOMParser::VR_DA:
    case DICOMParser::VR_DS:
    case DICOMParser::VR_DT:
    case DICOMParser::VR_IS:
    case DICOMParser::VR_LO:
    case DICOMParser::VR_LT:
    case DICOMParser::VR_PN:
    case DICOMParser::VR_ST:
    case DICOMParser::VR_TM:
    case DICOMParser::VR_SH:
    case DICOMParser::VR_FL:
    case DICOMParser::VR_SL:
    case DICOMParser::VR_AT:
    case DICOMParser::VR_UL:
    case DICOMParser::VR_US:
    case DICOMParser::VR_SS:
    case DICOMParser::VR_FD:
      len = DataFile->ReadDoubleByte();
      mytype = VRTypes(rep);
      return true;

    case DICOMParser::VR_OB: // OB - LE
    case DICOMParser::VR_OW:
    case DICOMParser::VR_UN:
    case DICOMParser::VR_UT:
    case DICOMParser::VR_SQ:
      DataFile->ReadDoubleByte();
      len = DataFile->ReadQuadByte();
      mytype = VRTypes(rep);
      return true;

    default:
      //
      //
      // Need to comment out in new paradigm.
      //
      DataFile->Skip(-2);
      len = DataFile->ReadQuadByte();
      mytype = DICOMParser::VR_UNKNOWN;
      return false;
  }
}

void DICOMParser::ReadNextRecord(
  doublebyte& group, doublebyte& element, DICOMParser::VRTypes& mytype)
{
  //
  // WE SHOULD IMPLEMENT THIS ALGORITHM.
  //
  // FIND A WAY TO STOP IF THERE ARE NO MORE CALLBACKS.
  //
  // DO WE NEED TO ENSURE THAT WHEN A CALLBACK IS ADDED THAT
  // THE IMPLICIT TYPE MAP IS UPDATED?  ONLY IF THERE ISN'T
  // A VALUE IN THE IMPLICIT TYPE MAP.
  //
  // New algorithm:
  //
  // 1. Read group & element
  // 2. ParseExplicitRecord
  //      a. Check to see if the next doublebyte is a valid datatype
  //      b. If the type is valid, lookup type to find the size of
  //              the length field.
  // 3.   If ParseExplicitRecord fails, ParseImplicitRecord
  //      a. Lookup implicit datatype
  // 4. Check to see if there is a registered callback for the group,element.
  // 5. If there are callbacks, read the data and call them, otherwise
  //      skip ahead to the next record.
  //
  //

  group = DataFile->ReadDoubleByte();
  element = DataFile->ReadDoubleByte();

  doublebyte representation = DataFile->ReadDoubleByteAsLittleEndian();
  quadbyte length = 0;
  mytype = DICOMParser::VR_UNKNOWN;
  this->IsValidRepresentation(representation, length, mytype);

  DICOMParserMap::iterator iter = Implementation->Map.find(DICOMMapKey(group, element));

  VRTypes callbackType;

  if (iter != Implementation->Map.end())
  {
    //
    // Only read the data if there's a registered callback.
    //
    unsigned char* tempdata =
      reinterpret_cast<unsigned char*>(DataFile->ReadAsciiCharArray(length));

    DICOMMapKey ge = (*iter).first;
    callbackType = VRTypes(((*iter).second.first));

    if (callbackType != mytype && mytype != VR_UNKNOWN)
    {
      //
      // mytype is not VR_UNKNOWN if the file is in Explicit format.
      //
      callbackType = mytype;
    }

    std::pair<const DICOMMapKey, DICOMMapValue> p = *iter;
    DICOMMapValue mv = p.second;

    bool doSwap = (this->ToggleByteSwapImageData ^ this->DataFile->GetPlatformIsBigEndian()) &&
      callbackType == VR_OW;

    if (group == 0x7FE0 && element == 0x0010)
    {
      if (doSwap)
      {
        size_t uLength = static_cast<size_t>(length);
        DICOMFile::swap2(reinterpret_cast<ushort*>(tempdata), reinterpret_cast<ushort*>(tempdata),
          static_cast<int>(uLength / sizeof(ushort)));
      }
    }
    else
    {
      if (this->DataFile->GetPlatformIsBigEndian())
      {
        size_t uLength = static_cast<size_t>(length);
        switch (callbackType)
        {
          case DICOMParser::VR_OW:
          case DICOMParser::VR_US:
          case DICOMParser::VR_SS:
            DICOMFile::swap2(reinterpret_cast<ushort*>(tempdata),
              reinterpret_cast<ushort*>(tempdata), static_cast<int>(uLength / sizeof(ushort)));
            // std::cout << "16 bit byte swap needed!" << std::endl;
            break;
          case DICOMParser::VR_FL:
          case DICOMParser::VR_FD:
            // No need to byte swap here, since these values were read by sscanf
            break;
          case DICOMParser::VR_SL:
          case DICOMParser::VR_UL:
            DICOMFile::swap4(reinterpret_cast<uint*>(tempdata), reinterpret_cast<uint*>(tempdata),
              static_cast<int>(uLength / sizeof(uint)));
            // std::cout << "32 bit byte swap needed!" << std::endl;
            break;
          case DICOMParser::VR_AT:
            // std::cout << "ATTRIBUTE Byte swap needed!" << std::endl;
            break;
          case VR_UNKNOWN:
          case VR_DA:
          case VR_OB:
          case VR_AE:
          case VR_SH:
          case VR_UI:
          case VR_TM:
          case VR_PN:
          case VR_UN:
          case VR_LO:
          case VR_SQ:
          case VR_AS:
          case VR_CS:
          case VR_DS:
          case VR_IS:
          case VR_DT:
          case VR_LT:
          case VR_ST:
          case VR_UT:
          case VR_AW:
          default:
            break;
        }
      }
    }

    std::vector<DICOMCallback*>* cbVector = mv.second;
    for (std::vector<DICOMCallback*>::iterator cbiter = cbVector->begin();
         cbiter != cbVector->end(); ++cbiter)
    {
      (*cbiter)->Execute(this, // parser
        ge.first,              // group
        ge.second,             // element
        callbackType,          // type
        tempdata,              // data
        length);               // length
    }

    delete[] tempdata;
  }
  else
  {
    //
    // Some lengths are negative, but we don't
    // want to back up the file pointer.
    //
    if (length > 0)
    {
      DataFile->Skip(length);
    }
  }
}

void DICOMParser::InitTypeMap()
{
  DICOMRecord dicom_tags[] = {
    { 0x0002, 0x0002, DICOMParser::VR_UI }, // Media storage SOP class uid
    { 0x0002, 0x0003, DICOMParser::VR_UI }, // Media storage SOP inst uid
    { 0x0002, 0x0010, DICOMParser::VR_UI }, // Transfer syntax uid
    { 0x0002, 0x0012, DICOMParser::VR_UI }, // Implementation class uid
    { 0x0008, 0x0018, DICOMParser::VR_UI }, // Image UID
    { 0x0008, 0x0020, DICOMParser::VR_DA }, // Series date
    { 0x0008, 0x0030, DICOMParser::VR_TM }, // Series time
    { 0x0008, 0x0060, DICOMParser::VR_SH }, // Modality
    { 0x0008, 0x0070, DICOMParser::VR_SH }, // Manufacturer
    { 0x0008, 0x1060, DICOMParser::VR_SH }, // Physician
    { 0x0018, 0x0050, DICOMParser::VR_FL }, // slice thickness
    { 0x0018, 0x0060, DICOMParser::VR_FL }, // kV
    { 0x0018, 0x0088, DICOMParser::VR_FL }, // slice spacing
    { 0x0018, 0x1100, DICOMParser::VR_SH }, // Recon diameter
    { 0x0018, 0x1151, DICOMParser::VR_FL }, // mA
    { 0x0018, 0x1210, DICOMParser::VR_SH }, // Recon kernel
    { 0x0020, 0x000d, DICOMParser::VR_UI }, // Study UID
    { 0x0020, 0x000e, DICOMParser::VR_UI }, // Series UID
    { 0x0020, 0x0013, DICOMParser::VR_IS }, // Image number
    { 0x0020, 0x0032, DICOMParser::VR_SH }, // Patient position
    { 0x0020, 0x0037, DICOMParser::VR_SH }, // Patient position cosines
    { 0x0028, 0x0010, DICOMParser::VR_US }, // Num rows
    { 0x0028, 0x0011, DICOMParser::VR_US }, // Num cols
    { 0x0028, 0x0030, DICOMParser::VR_FL }, // pixel spacing
    { 0x0028, 0x0100, DICOMParser::VR_US }, // Bits allocated
    { 0x0028, 0x0120, DICOMParser::VR_UL }, // pixel padding
    { 0x0028, 0x1052, DICOMParser::VR_FL }, // pixel offset
    { 0x7FE0, 0x0010, DICOMParser::VR_OW }  // pixel data
  };

  int num_tags = sizeof(dicom_tags) / sizeof(DICOMRecord);

  doublebyte group;
  doublebyte element;
  DICOMTypeValue datatype;

  for (int i = 0; i < num_tags; i++)
  {
    group = dicom_tags[i].group;
    element = dicom_tags[i].element;
    datatype = static_cast<DICOMTypeValue>(dicom_tags[i].datatype);
    Implementation->TypeMap.insert(
      std::pair<const DICOMMapKey, DICOMTypeValue>(DICOMMapKey(group, element), datatype));
  }
}

void DICOMParser::SetDICOMTagCallbacks(
  doublebyte group, doublebyte element, VRTypes datatype, std::vector<DICOMCallback*>* cbVector)
{
  Implementation->Map.insert(std::pair<const DICOMMapKey, DICOMMapValue>(
    DICOMMapKey(group, element), DICOMMapValue(static_cast<doublebyte>(datatype), cbVector)));
}

bool DICOMParser::CheckMagic(char* magic_number)
{
  return ((magic_number[0] == DICOM_MAGIC[0]) && (magic_number[1] == DICOM_MAGIC[1]) &&
    (magic_number[2] == DICOM_MAGIC[2]) && (magic_number[3] == DICOM_MAGIC[3]));
}

void DICOMParser::DumpTag(std::ostream& out, doublebyte group, doublebyte element, VRTypes vrtype,
  unsigned char* tempdata, quadbyte length)
{

  int t2 = int((0x0000FF00 & vrtype) >> 8);
  int t1 = int((0x000000FF & vrtype));

  if (t1 == 0 && t2 == 0)
  {
    t1 = '?';
    t2 = '?';
  }

  char ct2 = static_cast<char>(t2);
  char ct1 = static_cast<char>(t1);

  out << "(0x";

  out.width(4);
  char prev = out.fill('0');

  out << std::hex << group;
  out << ",0x";

  out.width(4);
  out.fill('0');

  out << std::hex << element;
  out << ") ";

  out.fill(prev);
  out << std::dec;
  out << " " << ct1 << ct2 << " ";
  out << "[" << length << " bytes] ";

  if (group == 0x7FE0 && element == 0x0010)
  {
    out << "Image data not printed.";
  }
  else
  {
    out << (tempdata ? reinterpret_cast<char*>(tempdata) : "nullptr");
  }

  out << std::dec << std::endl;
  out.fill(prev);
  out << std::dec;
}

void DICOMParser::ModalityTag(doublebyte, doublebyte, VRTypes, unsigned char* tempdata, quadbyte)
{
  if (!strcmp(reinterpret_cast<char*>(tempdata), "MR"))
  {
    // this->AddMRTags();
  }
  else if (!strcmp(reinterpret_cast<char*>(tempdata), "CT"))
  {
  }
  else if (!strcmp(reinterpret_cast<char*>(tempdata), "US"))
  {
  }
}

void DICOMParser::AddDICOMTagCallbacks(
  doublebyte group, doublebyte element, VRTypes datatype, std::vector<DICOMCallback*>* cbVector)
{
  DICOMParserMap::iterator miter = Implementation->Map.find(DICOMMapKey(group, element));
  if (miter != Implementation->Map.end())
  {
    for (std::vector<DICOMCallback*>::iterator iter = cbVector->begin(); iter != cbVector->end();
         ++iter)
    {
      std::vector<DICOMCallback*>* callbacks = (*miter).second.second;
      callbacks->push_back(*iter);
    }
  }
  else
  {
    this->SetDICOMTagCallbacks(group, element, datatype, cbVector);
  }
}

void DICOMParser::AddDICOMTagCallback(
  doublebyte group, doublebyte element, VRTypes datatype, DICOMCallback* cb)
{
  DICOMParserMap::iterator miter = Implementation->Map.find(DICOMMapKey(group, element));
  if (miter != Implementation->Map.end())
  {
    std::vector<DICOMCallback*>* callbacks = (*miter).second.second;
    callbacks->push_back(cb);
  }
  else
  {
    std::vector<DICOMCallback*>* callback = new std::vector<DICOMCallback*>;
    callback->push_back(cb);
    this->SetDICOMTagCallbacks(group, element, datatype, callback);
  }
}

void DICOMParser::AddDICOMTagCallbackToAllTags(DICOMCallback* cb)
{
  DICOMParserMap::iterator miter;
  for (miter = Implementation->Map.begin(); miter != Implementation->Map.end(); ++miter)
  {
    std::vector<DICOMCallback*>* callbacks = (*miter).second.second;
    callbacks->push_back(cb);
  }
}

bool DICOMParser::ParseExplicitRecord(doublebyte, doublebyte, quadbyte& length, VRTypes& represent)
{
  doublebyte representation = DataFile->ReadDoubleByte();

  bool valid = this->IsValidRepresentation(representation, length, represent);

  if (valid)
  {
    return true;
  }
  else
  {
    represent = VR_UNKNOWN;
    length = 0;
    return false;
  }
}

bool DICOMParser::ParseImplicitRecord(
  doublebyte group, doublebyte element, quadbyte& length, VRTypes& represent)
{
  DICOMImplicitTypeMap::iterator iter = Implementation->TypeMap.find(DICOMMapKey(group, element));
  represent = VRTypes((*iter).second);
  //
  // length?
  //
  length = DataFile->ReadQuadByte();
  return false;
}

void DICOMParser::TransferSyntaxCallback(
  DICOMParser*, doublebyte, doublebyte, DICOMParser::VRTypes, unsigned char* val, quadbyte)

{
  const char* TRANSFER_UID_EXPLICIT_BIG_ENDIAN = "1.2.840.10008.1.2.2";
  const char* TRANSFER_UID_GE_PRIVATE_IMPLICIT_BIG_ENDIAN = "1.2.840.113619.5.2";

  this->ToggleByteSwapImageData = false;

  if (strcmp(TRANSFER_UID_EXPLICIT_BIG_ENDIAN, reinterpret_cast<char*>(val)) == 0)
  {
    this->ToggleByteSwapImageData = true;
    //
    // Data byte order is big endian
    //
    // We're always reading little endian in the beginning,
    // so now we need to swap.
  }
  else if (strcmp(TRANSFER_UID_GE_PRIVATE_IMPLICIT_BIG_ENDIAN, reinterpret_cast<char*>(val)) == 0)
  {
    this->ToggleByteSwapImageData = true;
  }
}

void DICOMParser::GetGroupsElementsDatatypes(std::vector<doublebyte>& groups,
  std::vector<doublebyte>& elements, std::vector<DICOMParser::VRTypes>& datatypes)
{
  groups.clear();
  elements.clear();
  datatypes.clear();

  std::vector<doublebyte>::iterator giter;           // = this->Groups.begin();
  std::vector<doublebyte>::iterator eiter;           // = this->Elements.begin();
  std::vector<DICOMParser::VRTypes>::iterator diter; // = this->Datatypes.begin();

  for (giter = this->Implementation->Groups.begin(), eiter = this->Implementation->Elements.begin(),
      diter = this->Implementation->Datatypes.begin();
       (giter != this->Implementation->Groups.end()) &&
       (eiter != this->Implementation->Elements.end()) &&
       (diter != this->Implementation->Datatypes.end());
       ++giter, ++eiter, ++diter)
  {
    groups.push_back(*giter);
    elements.push_back(*eiter);
    datatypes.push_back(*diter);
  }
}

void DICOMParser::ClearAllDICOMTagCallbacks()
{
  DICOMParserMap::iterator mapIter;

  for (mapIter = this->Implementation->Map.begin(); mapIter != this->Implementation->Map.end();
       ++mapIter)
  {
    std::pair<const DICOMMapKey, DICOMMapValue> mapPair = *mapIter;
    DICOMMapValue mapVal = mapPair.second;
    std::vector<DICOMCallback*>* cbVector = mapVal.second;

    delete cbVector;
  }

  this->Implementation->Map.clear();
}

DICOMParser::DICOMParser(const DICOMParser&)
{
  std::cerr << "DICOMParser copy constructor should not be called!" << std::endl;
}

void DICOMParser::operator=(const DICOMParser&)
{
  std::cerr << "DICOMParser assignment operator should not be called!" << std::endl;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
