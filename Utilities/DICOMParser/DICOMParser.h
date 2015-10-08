/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMParser.h
  Language:  C++

  Copyright (c) 2003 Matt Turek
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __DICOMParser_h_
#define __DICOMParser_h_


#ifdef _MSC_VER
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#pragma warning ( disable : 4710 )
#pragma warning ( disable : 4702 )
#pragma warning ( push, 3 )
#endif


#include <map>
#include <utility>
#include <vector>

#include "DICOMConfig.h"
#include "DICOMFile.h"
#include "DICOMTypes.h"
#include "DICOMParserMap.h"

class DICOMCallback;
template <class T> class DICOMMemberCallback;

//DICOM_EXPIMP_TEMPLATE template class DICOM_EXPORT dicom_stl::vector<doublebyte>;

class DICOMParserImplementation;

//
// We should keep a map with the implicit types for the groups and elements
// separate from the callbacks.  We can use this for the implicit types.
//
//

class DICOM_EXPORT DICOMParser
{
 public:

  //
  // Default constructor
  //
  DICOMParser();

  //
  // Default destructor.
  //
  virtual ~DICOMParser();

  //
  // Opens a file and initializes the parser.
  //
  bool OpenFile(const dicom_stl::string& filename);

  //
  // Closes the currently open file.
  //
  void CloseFile();

  //
  // Return the name of the file last processed.
  //
  const dicom_stl::string& GetFileName();

  //
  // This method kicks off the parser.
  // OpenFile needs to be called first.
  //
  bool ReadHeader();

  //
  // Static method that returns true if DICOMFile is opened
  // to a file that contains a DICOM image.
  //
  static bool IsDICOMFile(DICOMFile* file);

  bool IsDICOMFile()
    {
    if (!this->DataFile)
      {
      return false;
      }
    return DICOMParser::IsDICOMFile(this->DataFile);
    }

  //
  // Static method that checks the DICOM magic number.
  //
  static bool CheckMagic(char* magic_number);

  //
  // Defined DICOM types.
  //
  enum VRTypes
    {
      VR_UNKNOWN = 0x0,
      VR_OB=0x424f, // Other byte string (string of bytes, insensitive to byte order)
      VR_AW=0x5741,
      VR_AE=0x4541, // Application Entity (char string)
      VR_AS=0x5341, // Age string (char string)
      VR_CS=0x5343, // Code string (char string, leading/trailing spaces insignificant)
      VR_UI=0x4955, // UID (character string)
      VR_DA=0x4144, // Date (char string)
      VR_DS=0x5344, // Decimal string (char string representing a fixed/floating pt number)
      VR_DT=0x5444, // Date Time (char string)
      VR_IS=0x5349, // Integer string (char string representing an integer)
      VR_LO=0x4f4c, // Long string (character string padded with leading/trailing spaces)
      VR_LT=0x544c, // Long text (character string with one or more paragraphs.)
      VR_OW=0x574f, // Other word string (sensitive to byte order, transfer syntax)
      VR_PN=0x4e50, // Person name (character string)
      VR_ST=0x5453, // Short text (character string, one or more paragraphs)
      VR_TM=0x4d54, // Time (character string)
      VR_UN=0x4e55,
      VR_UT=0x5455, // Unlimited text (character string)
      VR_SQ=0x5153, // Sequence of items
      VR_SH=0x4853, // Short string (character string with leading/trailing spaces).
      VR_FL=0x4c46, // Floating point single precision
      VR_SL=0x4c53, // Signed long, (signed 32bits, 2's complement form.)
      VR_AT=0x5441, // Attribute tag (ordered pair 16 bit unsigned)
      VR_UL=0x4c55, // Unsigned long (unsigned 32 bits)
      VR_US=0x5355, // Unsigned short (unsigned 16 bits)
      VR_SS=0x5353, // Signed short, (signed 16bits, 2's complement)
      VR_FD=0x4446  // Floating point double precision
    };

  //
  // Callback for the modality tag.
  //
  void ModalityTag(doublebyte group, doublebyte element, VRTypes datatype, unsigned char* tempdata, quadbyte length);

  void SetDICOMTagCallbacks(doublebyte group, doublebyte element, VRTypes datatype, dicom_stl::vector<DICOMCallback*>* cbVector);
  void AddDICOMTagCallbacks(doublebyte group, doublebyte element, VRTypes datatype, dicom_stl::vector<DICOMCallback*>* cbVector);
  void AddDICOMTagCallback (doublebyte group, doublebyte element, VRTypes datatype, DICOMCallback* cb);
  void AddDICOMTagCallbackToAllTags(DICOMCallback* cb);

  DICOMFile* GetDICOMFile()
    {
    return this->DataFile;
    }

  void ClearAllDICOMTagCallbacks();


  void TransferSyntaxCallback(DICOMParser *parser,
                              doublebyte,
                              doublebyte,
                              DICOMParser::VRTypes,
                              unsigned char* val,
                              quadbyte) ;

  void GetGroupsElementsDatatypes(dicom_stl::vector<doublebyte>& groups,
                                  dicom_stl::vector<doublebyte>& elements,
                                  dicom_stl::vector<VRTypes>& datatypes);

 protected:

  bool ParseExplicitRecord(doublebyte group, doublebyte element,
                           quadbyte& length,
                           VRTypes& represent);

  bool ParseImplicitRecord(doublebyte group, doublebyte element,
                           quadbyte& length,
                           VRTypes& represent);
  //
  // Print a tag.
  //
  // void DumpTag(doublebyte group, doublebyte element, VRTypes datatype, unsigned char* data, quadbyte length);
  void DumpTag(dicom_stream::ostream& out, doublebyte group, doublebyte element, VRTypes vrtype, unsigned char* tempdata, quadbyte length);

  struct DICOMRecord
    {
    doublebyte group;
    doublebyte element;
    VRTypes datatype;
    };

  //
  // Check to see if the type is a valid DICOM type.  If not, figure
  // out the right thing to do next (i.e. compute the element length).
  //
  bool IsValidRepresentation(doublebyte rep, quadbyte& len, VRTypes &mytype);

  //
  // Reads a record.
  //
  void ReadNextRecord(doublebyte& group, doublebyte& element, DICOMParser::VRTypes& mytype);

  //
  // Sets up the type map.
  //
  void InitTypeMap();

  //
  // Flags for byte swaping header values and
  // image data.
  //
  bool ByteSwap;
  bool ByteSwapData;

  //
  // Stores a map from pair<group, element> keys to
  // values of pair<vector<DICOMCallback*>, datatype>
  //
  // DICOMParserMap Map;

  //
  // Stores a map from pair<group, element> keys to
  // values of datatype.  We use this to store the
  // datatypes for implicit keys that we are
  // interested in.
  //
  // DICOMImplicitTypeMap TypeMap;

  //
  // Used for outputting debug information.
  //
  dicom_stream::ofstream ParserOutputFile;

  //
  // Pointer to the DICOMFile we're parsing.
  //
  DICOMFile* DataFile;
  dicom_stl::string FileName;

  bool ToggleByteSwapImageData;

  //dicom_stl::vector<doublebyte> Groups;
  //dicom_stl::vector<doublebyte> Elements;
  //dicom_stl::vector<VRTypes> Datatypes;

  DICOMMemberCallback<DICOMParser>* TransferSyntaxCB;

  //
  // Implementation contains stl templated classes that
  // can't be exported from a DLL in Windows. We hide
  // them in the implementation to get rid of annoying
  // compile warnings.
  //
  DICOMParserImplementation* Implementation;

 private:
  DICOMParser(const DICOMParser&);
  void operator=(const DICOMParser&);
};


#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif   // __DICOMParser_h_
