/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMParser.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2003 Matt Turek
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#ifdef _MSC_VER
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#pragma warning ( disable : 4710 )
#pragma warning ( push, 3 )
#endif

#include "DICOMConfig.h"
#include "DICOMParser.h"
#include "DICOMCallback.h"

#include <stdlib.h>
#if !defined(__MWERKS__)
#include <math.h>
#endif
#include <time.h>
#include <assert.h>
#if !defined(__MWERKS__)
#include <sys/types.h>
#endif

#include <string.h>
#include <string>

// Define DEBUG_DICOM to get debug messages sent to dicom_stream::cerr
// #define DEBUG_DICOM

#define DICOMPARSER_IGNORE_MAGIC_NUMBER

#ifdef DEBUG_DICOM
#define DICOM_DBG_MSG(x) {dicom_stream::cout x}
#else
#define DICOM_DBG_MSG(x)
#endif

static const char* DICOM_MAGIC = "DICM";
static const int   OPTIONAL_SKIP = 128;

class DICOMParserImplementation
{
public:
  DICOMParserImplementation() : Groups(), Elements(), Datatypes(), Map(), TypeMap()
  {

  };

  dicom_stl::vector<doublebyte> Groups;
  dicom_stl::vector<doublebyte> Elements;
  dicom_stl::vector<DICOMParser::VRTypes> Datatypes;
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

DICOMParser::DICOMParser() : ParserOutputFile()
{
  this->Implementation = new DICOMParserImplementation();
  this->DataFile = NULL;
  this->ToggleByteSwapImageData = false;
  this->TransferSyntaxCB = new DICOMMemberCallback<DICOMParser>;
  this->InitTypeMap();
  this->FileName = "";
}

const dicom_stl::string&
DICOMParser::GetFileName()
{
  return this->FileName;
}

bool DICOMParser::OpenFile(const dicom_stl::string& filename)
{
  if (this->DataFile)
    {
    // Deleting the DataFile closes the file
    delete this->DataFile;
    }
  this->DataFile = new DICOMFile();
  bool val = this->DataFile->Open(filename);

  if (val)
    {
    this->FileName = filename;
    }


#ifdef DEBUG_DICOM
  if (this->ParserOutputFile.rdbuf()->is_open())
    {
    this->ParserOutputFile.flush();
    this->ParserOutputFile.close();
    }

  dicom_stl::string fn(filename);
  dicom_stl::string append(".parser.txt");
  dicom_stl::string parseroutput(fn + append);
  // dicom_stl::string parseroutput(dicom_stl::string(dicom_stl::string(filename) + dicom_stl::string(".parser.txt")));
  this->ParserOutputFile.open(parseroutput.c_str()); //, dicom_stream::ios::app);
#endif

  return val;
}

DICOMParser::~DICOMParser() {
  //
  // Delete the callbacks.
  //
  this->ClearAllDICOMTagCallbacks();

  if (this->DataFile)
    {
    delete this->DataFile;
    }

  delete this->TransferSyntaxCB;
  delete this->Implementation;

#ifdef DEBUG_DICOM
  this->ParserOutputFile.flush();
  this->ParserOutputFile.close();
#endif

}

bool DICOMParser::ReadHeader() {
  bool dicom = this->IsDICOMFile(this->DataFile);
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
bool DICOMParser::IsDICOMFile(DICOMFile* file) {
  char magic_number[4];
  file->SkipToStart();
  file->Read(static_cast<void*>(magic_number),4);
  if (CheckMagic(magic_number))
    {
    return(true);
    }
  // try with optional skip
  else
    {
    file->Skip(OPTIONAL_SKIP-4);
    file->Read(static_cast<void*>(magic_number),4);
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
        dicom_stream::cerr
          << "No DICOM magic number found, but file appears to be DICOM."
          << dicom_stream::endl;
        dicom_stream::cerr << "Proceeding without caution."
          << dicom_stream::endl;
        dicom = true;
        }
      else
        {
        dicom = false;
        }
      file->SkipToStart();

      return dicom;
#endif  // DICOMPARSER_IGNORE_MAGIC_NUMBER

      }
    }
}


bool DICOMParser::IsValidRepresentation(doublebyte rep, quadbyte& len, VRTypes &mytype)
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
    case DICOMParser::VR_UT: // new
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

void DICOMParser::ReadNextRecord(doublebyte& group, doublebyte& element, DICOMParser::VRTypes& mytype)
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

  DICOMParserMap::iterator iter =
    Implementation->Map.find(DICOMMapKey(group,element));

  VRTypes callbackType;

  if (iter != Implementation->Map.end())
    {
    //
    // Only read the data if there's a registered callback.
    //
    unsigned char* tempdata = reinterpret_cast<unsigned char*>(
      DataFile->ReadAsciiCharArray(length));

    DICOMMapKey ge = (*iter).first;
    callbackType = VRTypes(((*iter).second.first));

    if (callbackType != mytype &&
        mytype != VR_UNKNOWN)
      {
      //
      // mytype is not VR_UNKNOWN if the file is in Explicit format.
      //
      callbackType = mytype;
      }

#ifdef DEBUG_DICOM
    this->DumpTag(this->ParserOutputFile, group, element, callbackType, tempdata, length);
#endif

    dicom_stl::pair<const DICOMMapKey,DICOMMapValue> p = *iter;
    DICOMMapValue mv = p.second;

    bool doSwap = (this->ToggleByteSwapImageData ^ this->DataFile->GetPlatformIsBigEndian()) && callbackType == VR_OW;

    if (group == 0x7FE0 &&
        element == 0x0010 )
      {
      if (doSwap)
        {
#ifdef DEBUG_DICOM
        dicom_stream::cout << "==============================" << dicom_stream::endl;
        dicom_stream::cout << "TOGGLE BS FOR IMAGE" << dicom_stream::endl;
        dicom_stream::cout << " ToggleByteSwapImageData : " << this->ToggleByteSwapImageData << dicom_stream::endl;
        dicom_stream::cout << " DataFile Byte Swap : " << this->DataFile->GetPlatformIsBigEndian() << dicom_stream::endl;
        dicom_stream::cout << "==============================" << dicom_stream::endl;
#endif
        size_t uLength=static_cast<size_t>(length);
        DICOMFile::swap2(reinterpret_cast<ushort*>(tempdata),
                         reinterpret_cast<ushort*>(tempdata),
                         static_cast<int>(uLength/sizeof(ushort)));
        }
      else
        {
#ifdef DEBUG_DICOM
        dicom_stream::cout << "==============================" << dicom_stream::endl;
        dicom_stream::cout << " AT IMAGE DATA " << dicom_stream::endl;
        dicom_stream::cout << " ToggleByteSwapImageData : " << this->ToggleByteSwapImageData << dicom_stream::endl;
        dicom_stream::cout << " DataFile Byte Swap : " << this->DataFile->GetPlatformIsBigEndian() << dicom_stream::endl;

        int t2 = int((0x0000FF00 & callbackType) >> 8);
        int t1 = int((0x000000FF & callbackType));

        if (t1 == 0 && t2 == 0)
          {
          t1 = '?';
          t2 = '?';
          }

        char ct2(t2);
        char ct1(t1);
        dicom_stream::cout << " Callback type : " << ct1 << ct2 << dicom_stream::endl;

        dicom_stream::cout << "==============================" << dicom_stream::endl;
#endif
        }
      }
    else
      {
      if (this->DataFile->GetPlatformIsBigEndian() == true)
        {
        size_t uLength=static_cast<size_t>(length);
        switch (callbackType)
          {
          case DICOMParser::VR_OW:
          case DICOMParser::VR_US:
          case DICOMParser::VR_SS:
            DICOMFile::swap2(reinterpret_cast<ushort*>(tempdata),
                             reinterpret_cast<ushort*>(tempdata),
                             static_cast<int>(uLength/sizeof(ushort)));
            // dicom_stream::cout << "16 bit byte swap needed!" << dicom_stream::endl;
            break;
          case DICOMParser::VR_FL:
          case DICOMParser::VR_FD:
            // No need to byte swap here, since these values were read by sscanf
            break;
          case DICOMParser::VR_SL:
          case DICOMParser::VR_UL:
            DICOMFile::swap4(reinterpret_cast<uint*>(tempdata),
                             reinterpret_cast<uint*>(tempdata),
                             static_cast<int>(uLength/sizeof(uint)));
            // dicom_stream::cout << "32 bit byte swap needed!" << dicom_stream::endl;
            break;
          case DICOMParser::VR_AT:
            // dicom_stream::cout << "ATTRIBUTE Byte swap needed!" << dicom_stream::endl;
            break;
          default:
            break;
          }
        }
      }

    dicom_stl::vector<DICOMCallback*> * cbVector = mv.second;
    for (dicom_stl::vector<DICOMCallback*>::iterator cbiter = cbVector->begin();
         cbiter != cbVector->end();
         cbiter++)
      {
      (*cbiter)->Execute(this,      // parser
                       ge.first,  // group
                       ge.second,  // element
                       callbackType,  // type
                       tempdata, // data
                       length);  // length
      }

    delete [] tempdata;
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
#ifdef DEBUG_DICOM
    this->DumpTag(this->ParserOutputFile, group, element, mytype, (unsigned char*) "Unread.", length);
#endif
  }


}

void DICOMParser::InitTypeMap()
{
  DICOMRecord dicom_tags[] = {{0x0002, 0x0002, DICOMParser::VR_UI}, // Media storage SOP class uid
                              {0x0002, 0x0003, DICOMParser::VR_UI}, // Media storage SOP inst uid
                              {0x0002, 0x0010, DICOMParser::VR_UI}, // Transfer syntax uid
                              {0x0002, 0x0012, DICOMParser::VR_UI}, // Implementation class uid
                              {0x0008, 0x0018, DICOMParser::VR_UI}, // Image UID
                              {0x0008, 0x0020, DICOMParser::VR_DA}, // Series date
                              {0x0008, 0x0030, DICOMParser::VR_TM}, // Series time
                              {0x0008, 0x0060, DICOMParser::VR_SH}, // Modality
                              {0x0008, 0x0070, DICOMParser::VR_SH}, // Manufacturer
                              {0x0008, 0x1060, DICOMParser::VR_SH}, // Physician
                              {0x0018, 0x0050, DICOMParser::VR_FL}, // slice thickness
                              {0x0018, 0x0060, DICOMParser::VR_FL}, // kV
                              {0x0018, 0x0088, DICOMParser::VR_FL}, // slice spacing
                              {0x0018, 0x1100, DICOMParser::VR_SH}, // Recon diameter
                              {0x0018, 0x1151, DICOMParser::VR_FL}, // mA
                              {0x0018, 0x1210, DICOMParser::VR_SH}, // Recon kernel
                              {0x0020, 0x000d, DICOMParser::VR_UI}, // Study UID
                              {0x0020, 0x000e, DICOMParser::VR_UI}, // Series UID
                              {0x0020, 0x0013, DICOMParser::VR_IS}, // Image number
                              {0x0020, 0x0032, DICOMParser::VR_SH}, // Patient position
                              {0x0020, 0x0037, DICOMParser::VR_SH}, // Patient position cosines
                              {0x0028, 0x0010, DICOMParser::VR_US}, // Num rows
                              {0x0028, 0x0011, DICOMParser::VR_US}, // Num cols
                              {0x0028, 0x0030, DICOMParser::VR_FL}, // pixel spacing
                              {0x0028, 0x0100, DICOMParser::VR_US}, // Bits allocated
                              {0x0028, 0x0120, DICOMParser::VR_UL}, // pixel padding
                              {0x0028, 0x1052, DICOMParser::VR_FL}, // pixel offset
                              {0x7FE0, 0x0010, DICOMParser::VR_OW}   // pixel data
  };


  int num_tags = sizeof(dicom_tags)/sizeof(DICOMRecord);

  doublebyte group;
  doublebyte element;
  VRTypes datatype;

  for (int i = 0; i < num_tags; i++)
    {
    group = dicom_tags[i].group;
    element = dicom_tags[i].element;
    datatype = static_cast<VRTypes>(dicom_tags[i].datatype);
    Implementation->TypeMap.insert(dicom_stl::pair<const DICOMMapKey, DICOMTypeValue>(DICOMMapKey(group, element), datatype));
    }

}

void DICOMParser::SetDICOMTagCallbacks(doublebyte group, doublebyte element, VRTypes datatype, dicom_stl::vector<DICOMCallback*>* cbVector)
{
  Implementation->Map.insert(dicom_stl::pair<const DICOMMapKey, DICOMMapValue>(DICOMMapKey(group, element), DICOMMapValue(static_cast<doublebyte>(datatype), cbVector)));
}


bool DICOMParser::CheckMagic(char* magic_number)
{
  return (
          (magic_number[0] == DICOM_MAGIC[0]) &&
          (magic_number[1] == DICOM_MAGIC[1]) &&
          (magic_number[2] == DICOM_MAGIC[2]) &&
          (magic_number[3] == DICOM_MAGIC[3])
          );
}

void DICOMParser::DumpTag(dicom_stream::ostream& out, doublebyte group, doublebyte element, VRTypes vrtype, unsigned char* tempdata, quadbyte length)
{

  int t2 = int((0x0000FF00 & vrtype) >> 8);
  int t1 = int((0x000000FF & vrtype));

  if (t1 == 0 && t2 == 0)
    {
    t1 = '?';
    t2 = '?';
    }

  char ct2=static_cast<char>(t2);
  char ct1=static_cast<char>(t1);

  out << "(0x";

  out.width(4);
  char prev = out.fill('0');

  out << dicom_stream::hex << group;
  out << ",0x";

  out.width(4);
  out.fill('0');

  out << dicom_stream::hex << element;
  out << ") ";

  out.fill(prev);
  out << dicom_stream::dec;
  out << " " << ct1 << ct2 << " ";
  out << "[" << length << " bytes] ";

  if (group == 0x7FE0 && element == 0x0010)
    {
    out << "Image data not printed." ;
    }
  else
    {
    out << (tempdata ? reinterpret_cast<char*>(tempdata) : "NULL");
    }

  out << dicom_stream::dec << dicom_stream::endl;
  out.fill(prev);
  out << dicom_stream::dec;

  return;

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

void DICOMParser::AddDICOMTagCallbacks(doublebyte group, doublebyte element, VRTypes datatype, dicom_stl::vector<DICOMCallback*>* cbVector)
{
  DICOMParserMap::iterator miter = Implementation->Map.find(DICOMMapKey(group,element));
  if (miter != Implementation->Map.end())
    {
    for (dicom_stl::vector<DICOMCallback*>::iterator iter = cbVector->begin();
         iter != cbVector->end();
         iter++)
      {
      dicom_stl::vector<DICOMCallback*>* callbacks = (*miter).second.second;
      callbacks->push_back(*iter);
      }
    }
  else
    {
    this->SetDICOMTagCallbacks(group, element, datatype, cbVector);
    }
}

void DICOMParser::AddDICOMTagCallback(doublebyte group, doublebyte element, VRTypes datatype, DICOMCallback* cb)
{
  DICOMParserMap::iterator miter = Implementation->Map.find(DICOMMapKey(group,element));
  if (miter != Implementation->Map.end())
    {
    dicom_stl::vector<DICOMCallback*>* callbacks = (*miter).second.second;
    callbacks->push_back(cb);
    }
  else
    {
    dicom_stl::vector<DICOMCallback*>* callback = new dicom_stl::vector<DICOMCallback*>;
    callback->push_back(cb);
    this->SetDICOMTagCallbacks(group, element, datatype, callback);
    }
}

void DICOMParser::AddDICOMTagCallbackToAllTags(DICOMCallback* cb)
{
  DICOMParserMap::iterator miter;
  for (miter = Implementation->Map.begin();
       miter != Implementation->Map.end();
       miter++)
  {
  dicom_stl::vector<DICOMCallback*>* callbacks = (*miter).second.second;
  callbacks->push_back(cb);
  }
}

bool DICOMParser::ParseExplicitRecord(doublebyte, doublebyte,
                                      quadbyte& length,
                                      VRTypes& represent)
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

bool DICOMParser::ParseImplicitRecord(doublebyte group, doublebyte element,
                                      quadbyte& length,
                                      VRTypes& represent)
{
  DICOMImplicitTypeMap::iterator iter =
    Implementation->TypeMap.find(DICOMMapKey(group,element));
  represent = VRTypes((*iter).second);
  //
  // length?
  //
  length = DataFile->ReadQuadByte();
  return false;
}



void DICOMParser::TransferSyntaxCallback(DICOMParser *,
                                         doublebyte,
                                         doublebyte,
                                         DICOMParser::VRTypes,
                                         unsigned char* val,
                                         quadbyte)

{
#ifdef DEBUG_DICOM
  dicom_stream::cout << "DICOMParser::TransferSyntaxCallback" << dicom_stream::endl;
#endif

  const char* TRANSFER_UID_EXPLICIT_BIG_ENDIAN = "1.2.840.10008.1.2.2";
  const char* TRANSFER_UID_GE_PRIVATE_IMPLICIT_BIG_ENDIAN = "1.2.840.113619.5.2";

  // char* fileEndian = "LittleEndian";
  // char* dataEndian = "LittleEndian";

  this->ToggleByteSwapImageData = false;

  if (strcmp(TRANSFER_UID_EXPLICIT_BIG_ENDIAN,
             reinterpret_cast<char*>(val)) == 0)
    {
#ifdef DEBUG_DICOM
    dicom_stream::cout << "EXPLICIT BIG ENDIAN" << dicom_stream::endl;
#endif
    this->ToggleByteSwapImageData = true;
    //
    // Data byte order is big endian
    //
    // We're always reading little endian in the beginning,
    // so now we need to swap.
    }
  else if (strcmp(TRANSFER_UID_GE_PRIVATE_IMPLICIT_BIG_ENDIAN,
                  reinterpret_cast<char*>(val)) == 0)
    {
    this->ToggleByteSwapImageData = true;
#ifdef DEBUG_DICOM
    dicom_stream::cout << "GE PRIVATE TRANSFER SYNTAX" << dicom_stream::endl;
    dicom_stream::cout << "ToggleByteSwapImageData : " << this->ToggleByteSwapImageData << dicom_stream::endl;
#endif
    }
  else
    {
    }
}

void DICOMParser::GetGroupsElementsDatatypes(dicom_stl::vector<doublebyte>& groups,
                                             dicom_stl::vector<doublebyte>& elements,
                                             dicom_stl::vector<DICOMParser::VRTypes>& datatypes)
{
  groups.clear();
  elements.clear();
  datatypes.clear();

  dicom_stl::vector<doublebyte>::iterator giter; // = this->Groups.begin();
  dicom_stl::vector<doublebyte>::iterator eiter; // = this->Elements.begin();
  dicom_stl::vector<DICOMParser::VRTypes>::iterator diter; // = this->Datatypes.begin();

  for (giter = this->Implementation->Groups.begin(), eiter = this->Implementation->Elements.begin(), diter = this->Implementation->Datatypes.begin();
       (giter != this->Implementation->Groups.end()) && (eiter != this->Implementation->Elements.end()) && (diter != this->Implementation->Datatypes.end());
       giter++, eiter++, diter++)
    {
    groups.push_back(*giter);
    elements.push_back(*eiter);
    datatypes.push_back(*diter);
    }
}

void DICOMParser::ClearAllDICOMTagCallbacks()
{
  DICOMParserMap::iterator mapIter;

  for (mapIter = this->Implementation->Map.begin();
       mapIter != this->Implementation->Map.end();
       mapIter++)
       {
       dicom_stl::pair<const DICOMMapKey, DICOMMapValue> mapPair = *mapIter;
       DICOMMapValue mapVal = mapPair.second;
       dicom_stl::vector<DICOMCallback*>* cbVector = mapVal.second;

       delete cbVector;
       }

  this->Implementation->Map.clear();
}

DICOMParser::DICOMParser(const DICOMParser&)
{
  dicom_stream::cerr << "DICOMParser copy constructor should not be called!" << dicom_stream::endl;
}

void DICOMParser::operator=(const DICOMParser&)
{
  dicom_stream::cerr << "DICOMParser assignment operator should not be called!" << dicom_stream::endl;
}

#ifdef _MSC_VER
#pragma warning ( pop )
#endif
