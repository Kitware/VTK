/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMAppHelper.cxx
  Language:  C++

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
#pragma warning ( disable : 4702 )
#pragma warning ( push, 3 )
#endif

#include "DICOMConfig.h"
#include "DICOMAppHelper.h"
#include "DICOMCallback.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <math.h>
#include <algorithm>
#if defined(__BORLANDC__)
#include <mem.h> // for memcpy
#endif

//#define DEBUG_DICOM_APP_HELPER

class DICOMAppHelperImplementation
{
public:
  // map from series UID to vector of files in the series
  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr> SeriesUIDMap;

  // map from filename to intraseries sortable tags
  dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr> SliceOrderingMap;

  typedef dicom_stl::map<dicom_stl::pair<doublebyte, doublebyte>, DICOMTagInfo> TagMapType;
  TagMapType TagMap;

};

struct lt_pair_int_string
{
  bool operator()(const dicom_stl::pair<int, dicom_stl::string> &s1,
                  const dicom_stl::pair<int, dicom_stl::string> &s2) const
  {
    return s1.first < s2.first;
  }
};


struct lt_pair_float_string
{
  bool operator()(const dicom_stl::pair<float, dicom_stl::string> &s1,
                  const dicom_stl::pair<float, dicom_stl::string> &s2) const
  {
    return s1.first < s2.first;
  }
};


struct gt_pair_int_string
{
  bool operator()(const dicom_stl::pair<int, dicom_stl::string> &s1,
                  const dicom_stl::pair<int, dicom_stl::string> &s2) const
  {
    return s1.first > s2.first;
  }
};


struct gt_pair_float_string
{
  bool operator()(const dicom_stl::pair<float, dicom_stl::string> &s1,
                  const dicom_stl::pair<float, dicom_stl::string> &s2) const
  {
    return s1.first > s2.first;
  }
};


DICOMAppHelper::DICOMAppHelper()
{
  this->BitsAllocated = 8;
  this->ByteSwapData = false;
  this->PixelSpacing[0] = this->PixelSpacing[1] = this->PixelSpacing[2] = 1.0;
  this->Dimensions[0] = this->Dimensions[1] = 0;
  this->PhotometricInterpretation = new dicom_stl::string();
  this->TransferSyntaxUID = new dicom_stl::string();
  this->RescaleOffset = 0.0;
  this->RescaleSlope = 1.0;
  this->ImageData = NULL;
  this->ImageDataLengthInBytes = 0;
  this->PatientName = new dicom_stl::string();
  this->StudyUID = new dicom_stl::string();
  this->StudyID = new dicom_stl::string();
  this->GantryAngle = 0.0;
  this->Width = 0;
  this->Height = 0;
  this->PixelRepresentation = 0;

  this->SeriesUIDCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->SliceNumberCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->SliceLocationCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->ImagePositionPatientCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->ImageOrientationPatientCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->TransferSyntaxCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->ToggleSwapBytesCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->BitsAllocatedCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->PixelSpacingCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->HeightCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->WidthCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->PixelRepresentationCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->PhotometricInterpretationCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->RescaleOffsetCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->RescaleSlopeCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->PixelDataCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->PatientNameCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->StudyUIDCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->StudyIDCB = new DICOMMemberCallback<DICOMAppHelper>;
  this->GantryAngleCB = new DICOMMemberCallback<DICOMAppHelper>;

  this->Implementation = new DICOMAppHelperImplementation;
}

DICOMAppHelper::~DICOMAppHelper()
{
  this->Clear();

  this->HeaderFile.close();

  //
  // Fix warning here.
  //
  delete [] (static_cast<char*> (this->ImageData));

  delete this->TransferSyntaxUID;
  delete this->PhotometricInterpretation;
  delete this->PatientName;
  delete this->StudyUID;
  delete this->StudyID;
  delete this->SeriesUIDCB;
  delete this->SliceNumberCB;
  delete this->SliceLocationCB;
  delete this->ImagePositionPatientCB;
  delete this->ImageOrientationPatientCB;
  delete this->TransferSyntaxCB;
  delete this->ToggleSwapBytesCB;
  delete this->BitsAllocatedCB;
  delete this->PixelSpacingCB;
  delete this->HeightCB;
  delete this->WidthCB;
  delete this->PixelRepresentationCB;
  delete this->PhotometricInterpretationCB;
  delete this->RescaleOffsetCB;
  delete this->RescaleSlopeCB;
  delete this->PixelDataCB;
  delete this->PatientNameCB;
  delete this->StudyUIDCB;
  delete this->StudyIDCB;
  delete this->GantryAngleCB;

  delete this->Implementation;
}

void DICOMAppHelper::RegisterCallbacks(DICOMParser* parser)
{
  if (!parser)
    {
    dicom_stream::cerr << "Null parser!" << dicom_stream::endl;
    return;
    }

  SeriesUIDCB->SetCallbackFunction(this, &DICOMAppHelper::SeriesUIDCallback);
  parser->AddDICOMTagCallback(0x0020, 0x000e, DICOMParser::VR_UI, SeriesUIDCB);

  SliceNumberCB->SetCallbackFunction(this, &DICOMAppHelper::SliceNumberCallback);
  parser->AddDICOMTagCallback(0x0020, 0x0013, DICOMParser::VR_IS, SliceNumberCB);

  SliceLocationCB->SetCallbackFunction(this, &DICOMAppHelper::SliceLocationCallback);
  parser->AddDICOMTagCallback(0x0020, 0x1041, DICOMParser::VR_CS, SliceLocationCB);

  ImagePositionPatientCB->SetCallbackFunction(this, &DICOMAppHelper::ImagePositionPatientCallback);
  parser->AddDICOMTagCallback(0x0020, 0x0032, DICOMParser::VR_SH, ImagePositionPatientCB);

  ImageOrientationPatientCB->SetCallbackFunction(this, &DICOMAppHelper::ImageOrientationPatientCallback);
  parser->AddDICOMTagCallback(0x0020, 0x0037, DICOMParser::VR_SH, ImageOrientationPatientCB);

  TransferSyntaxCB->SetCallbackFunction(this, &DICOMAppHelper::TransferSyntaxCallback);
  parser->AddDICOMTagCallback(0x0002, 0x0010, DICOMParser::VR_UI, TransferSyntaxCB);

  ToggleSwapBytesCB->SetCallbackFunction(this, &DICOMAppHelper::ToggleSwapBytesCallback);

  BitsAllocatedCB->SetCallbackFunction(this, &DICOMAppHelper::BitsAllocatedCallback);
  parser->AddDICOMTagCallback(0x0028, 0x0100, DICOMParser::VR_US, BitsAllocatedCB);

  PixelSpacingCB->SetCallbackFunction(this, &DICOMAppHelper::PixelSpacingCallback);
  // Why is 0028,0030 VR:FL, it is VR::DS as per PS 3.6-2008 ...
  parser->AddDICOMTagCallback(0x0028, 0x0030, DICOMParser::VR_FL, PixelSpacingCB);
  parser->AddDICOMTagCallback(0x0018, 0x0050, DICOMParser::VR_FL, PixelSpacingCB);

  WidthCB->SetCallbackFunction(this, &DICOMAppHelper::WidthCallback);
  parser->AddDICOMTagCallback(0x0028, 0x0011, DICOMParser::VR_US, WidthCB);

  HeightCB->SetCallbackFunction(this, &DICOMAppHelper::HeightCallback);
  parser->AddDICOMTagCallback(0x0028, 0x0010, DICOMParser::VR_US, HeightCB);

  PixelRepresentationCB->SetCallbackFunction(this, &DICOMAppHelper::PixelRepresentationCallback);
  parser->AddDICOMTagCallback(0x0028, 0x0103, DICOMParser::VR_US, PixelRepresentationCB);

  PhotometricInterpretationCB->SetCallbackFunction(this, &DICOMAppHelper::PhotometricInterpretationCallback);
  parser->AddDICOMTagCallback(0x0028, 0x0004, DICOMParser::VR_CS, PhotometricInterpretationCB);

  RescaleOffsetCB->SetCallbackFunction(this, &DICOMAppHelper::RescaleOffsetCallback);
  parser->AddDICOMTagCallback(0x0028, 0x1052, DICOMParser::VR_CS, RescaleOffsetCB);

  RescaleSlopeCB->SetCallbackFunction(this, &DICOMAppHelper::RescaleSlopeCallback);
  parser->AddDICOMTagCallback(0x0028, 0x1053, DICOMParser::VR_FL, RescaleSlopeCB);

  PatientNameCB->SetCallbackFunction(this, &DICOMAppHelper::PatientNameCallback);
  parser->AddDICOMTagCallback(0x0010, 0x0010, DICOMParser::VR_PN, PatientNameCB);

  StudyUIDCB->SetCallbackFunction(this, &DICOMAppHelper::StudyUIDCallback);
  parser->AddDICOMTagCallback(0x0020, 0x000d, DICOMParser::VR_UI, StudyUIDCB);

  StudyIDCB->SetCallbackFunction(this, &DICOMAppHelper::StudyIDCallback);
  parser->AddDICOMTagCallback(0x0020, 0x0010, DICOMParser::VR_SH, StudyIDCB);

  GantryAngleCB->SetCallbackFunction(this, &DICOMAppHelper::GantryAngleCallback);
  parser->AddDICOMTagCallback(0x0018, 0x1120, DICOMParser::VR_FL, GantryAngleCB);


  DICOMTagInfo dicom_tags[] = {
    {0x0002, 0x0002, DICOMParser::VR_UI, "Media storage SOP class uid"},
    {0x0002, 0x0003, DICOMParser::VR_UI, "Media storage SOP inst uid"},
    {0x0002, 0x0010, DICOMParser::VR_UI, "Transfer syntax uid"},
    {0x0002, 0x0012, DICOMParser::VR_UI, "Implementation class uid"},
    {0x0008, 0x0018, DICOMParser::VR_UI, "Image UID"},
    {0x0008, 0x0020, DICOMParser::VR_DA, "Series date"},
    {0x0008, 0x0030, DICOMParser::VR_TM, "Series time"},
    {0x0008, 0x0060, DICOMParser::VR_SH, "Modality"},
    {0x0008, 0x0070, DICOMParser::VR_SH, "Manufacturer"},
    {0x0008, 0x1060, DICOMParser::VR_SH, "Physician"},
    {0x0018, 0x0050, DICOMParser::VR_FL, "slice thickness"},
    {0x0018, 0x0060, DICOMParser::VR_FL, "kV"},
    {0x0018, 0x0088, DICOMParser::VR_FL, "slice spacing"},
    {0x0018, 0x1100, DICOMParser::VR_SH, "Recon diameter"},
    {0x0018, 0x1151, DICOMParser::VR_FL, "mA"},
    {0x0018, 0x1210, DICOMParser::VR_SH, "Recon kernel"},
    {0x0020, 0x000d, DICOMParser::VR_UI, "Study UID"},
    {0x0020, 0x000e, DICOMParser::VR_UI, "Series UID"},
    {0x0020, 0x0013, DICOMParser::VR_IS, "Image number"},
    {0x0020, 0x0032, DICOMParser::VR_SH, "Patient position"},
    {0x0020, 0x0037, DICOMParser::VR_SH, "Patient position cosines"},
    {0x0020, 0x1041, DICOMParser::VR_CS, "Slice location"},
    {0x0028, 0x0010, DICOMParser::VR_FL, "Num rows"},
    {0x0028, 0x0011, DICOMParser::VR_FL, "Num cols"},
    {0x0028, 0x0030, DICOMParser::VR_FL, "pixel spacing"},
    {0x0028, 0x0100, DICOMParser::VR_US, "Bits allocated"},
    {0x0028, 0x0120, DICOMParser::VR_UL, "pixel padding"},
    {0x0028, 0x1052, DICOMParser::VR_FL, "pixel offset"}
  };

  int num_tags = sizeof(dicom_tags)/sizeof(DICOMTagInfo);

#ifdef DEBUG_DICOM_APP_HELPER
  DICOMMemberCallback<DICOMAppHelper>** callbackArray = new DICOMMemberCallback<DICOMAppHelper>*[num_tags];
#endif

  for (int j = 0; j < num_tags; j++)
    {
    //
    // Setup internal map.
    //
    DICOMTagInfo tagStruct = dicom_tags[j];
    doublebyte group = tagStruct.group;
    doublebyte element = tagStruct.element;

    dicom_stl::pair<doublebyte, doublebyte> gePair(group, element);
    dicom_stl::pair<const dicom_stl::pair<doublebyte, doublebyte>, DICOMTagInfo> mapPair(gePair, tagStruct);
    this->Implementation->TagMap.insert(mapPair);

#ifdef DEBUG_DICOM_APP_HELPER
    //
    // Make callback
    //
    callbackArray[j] = new DICOMMemberCallback<DICOMAppHelper>;
    callbackArray[j]->SetCallbackFunction(this, &DICOMAppHelper::ArrayCallback);
    //
    // Set callback on parser.
    //
    parser->AddDICOMTagCallback(group, element,datatype, callbackArray[j]);
#endif

    }

}

void DICOMAppHelper::SeriesUIDCallback(DICOMParser *parser,
                                       doublebyte,
                                       doublebyte,
                                       DICOMParser::VRTypes,
                                       unsigned char* val,
                                       quadbyte)
{
  char* newString = reinterpret_cast<char*>(val);
  dicom_stl::string newStdString(newString);
  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr>::iterator iter = this->Implementation->SeriesUIDMap.find(newStdString);
  if ( iter == this->Implementation->SeriesUIDMap.end())
    {
    dicom_stl::vector<dicom_stl::string> newVector;

    newVector.push_back(parser->GetFileName());
    this->Implementation->SeriesUIDMap.insert(dicom_stl::pair<const dicom_stl::string, dicom_stl::vector<dicom_stl::string> > (newStdString, newVector));
    }
  else
    {
    (*iter).second.push_back(parser->GetFileName());
    }
}

void DICOMAppHelper::OutputSeries()
{
  dicom_stream::cout << dicom_stream::endl << dicom_stream::endl;

  for (dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr >::iterator iter = this->Implementation->SeriesUIDMap.begin();
       iter != this->Implementation->SeriesUIDMap.end();
       iter++)
    {
    dicom_stream::cout << "SERIES: " << (*iter).first.c_str() << dicom_stream::endl;
    dicom_stl::vector<dicom_stl::string>& v_ref = (*iter).second;

    for (dicom_stl::vector<dicom_stl::string>::iterator v_iter = v_ref.begin();
         v_iter != v_ref.end();
         v_iter++)
      {
      dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator sn_iter = Implementation->SliceOrderingMap.find(*v_iter);

      int slice = -1;
      if (sn_iter != Implementation->SliceOrderingMap.end())
        {
        slice = (*sn_iter).second.SliceNumber;
        }
      dicom_stream::cout << "\t" << (*v_iter).c_str() << " [" << slice << "]" <<  dicom_stream::endl;
      }
    }
}


void DICOMAppHelper::ArrayCallback(DICOMParser *parser,
                                   doublebyte group,
                                   doublebyte element,
                                   DICOMParser::VRTypes datatype,
                                   unsigned char* val,
                                   quadbyte len)
{
  const char* desc = "No description";

  TagMapType::iterator iter = this->Implementation->TagMap.find(dicom_stl::pair<doublebyte, doublebyte> (group, element));
  if (iter != this->Implementation->TagMap.end())
    {
    desc = (*iter).second.description;
    }

  int t2 = int((0x0000FF00 & datatype) >> 8);
  int t1 = int((0x000000FF & datatype));

  char ct2=static_cast<char>(t2);
  char ct1=static_cast<char>(t1);


  HeaderFile << "(0x";

  HeaderFile.width(4);
  char prev = HeaderFile.fill('0');

  HeaderFile << dicom_stream::hex << group;
  HeaderFile << ",0x";

  HeaderFile.width(4);
  HeaderFile.fill('0');

  HeaderFile << dicom_stream::hex << element;
  HeaderFile << ") ";

  HeaderFile.fill(prev);
  HeaderFile << dicom_stream::dec;
  HeaderFile << " " << ct1 << ct2 << " ";
  HeaderFile << "[" << len << " bytes] ";

  HeaderFile << desc << " : ";

  unsigned int uival = 0;
  float fval = 0;
  double dval = 0;
  int ival = 0;

  if (val)
    {
    switch (datatype)
      {
      case DICOMParser::VR_AE:
      case DICOMParser::VR_AS:
      case DICOMParser::VR_CS:
      case DICOMParser::VR_UI:
      case DICOMParser::VR_DA:
      case DICOMParser::VR_DS:
      case DICOMParser::VR_DT:
      case DICOMParser::VR_LO:
      case DICOMParser::VR_LT:
      case DICOMParser::VR_OB: // ordered bytes
      case DICOMParser::VR_OW: // ordered words
      case DICOMParser::VR_PN:
      case DICOMParser::VR_ST:
      case DICOMParser::VR_TM:
      case DICOMParser::VR_UN:
      case DICOMParser::VR_UT:
      case DICOMParser::VR_SQ: // sequence
      case DICOMParser::VR_SH: // strings
      case DICOMParser::VR_IS:
        HeaderFile << val;
        break;
      case DICOMParser::VR_FL: // float
        fval = static_cast<float> (atof(reinterpret_cast<char*>(val)));
        HeaderFile << fval;
        break;
      case DICOMParser::VR_FD: // float double
        dval = static_cast<double> (atof(reinterpret_cast<char*>(val)));
        HeaderFile << dval;
        break;
      case DICOMParser::VR_UL: // unsigned long
      case DICOMParser::VR_SL: // signed long
      case DICOMParser::VR_AT:
        HeaderFile << uival;
        break;
      //case DICOMParser::VR_IS:
      //  ival = DICOMFile::ReturnAsSignedLong(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
      //  HeaderFile << ival;
      //  break;
      case DICOMParser::VR_SS:
        ival = DICOMFile::ReturnAsSignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
        HeaderFile << ival;
        break;
      case DICOMParser::VR_US: // unsigned short
        uival = DICOMFile::ReturnAsUnsignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
        HeaderFile << uival;
        break;
      case DICOMParser::VR_UNKNOWN:
      case DICOMParser::VR_AW:
      default:
        HeaderFile << val << dicom_stream::endl;
        break;
      }
    }
  else
    {
    HeaderFile << "NULL";
    }

  HeaderFile << dicom_stream::dec << dicom_stream::endl;
  HeaderFile.fill(prev);

  delete [] val;
}

void DICOMAppHelper::SliceNumberCallback(DICOMParser *parser,
                                         doublebyte,
                                         doublebyte,
                                         DICOMParser::VRTypes,
                                         unsigned char* val,
                                         quadbyte)
{
  // Look for the current file in the map of slice ordering data
  dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator it;
  it = this->Implementation->SliceOrderingMap.find(parser->GetFileName());
  if (it == Implementation->SliceOrderingMap.end())
    {
    // file not found, create a new entry
    DICOMOrderingElements ord;
    if( val )
      {
      ord.SliceNumber = atoi(reinterpret_cast<char *>(val));
      }
    else // Slice Number present but empty
      {
      ord.SliceNumber = 0;
      }

    // insert into the map
    this->Implementation->SliceOrderingMap.insert(
      dicom_stl::pair<const dicom_stl::string,
      DICOMOrderingElements>(parser->GetFileName(), ord));
    }
  else
    {
    // file found, add new values
    if( val )
      {
      (*it).second.SliceNumber = atoi(reinterpret_cast<char *>(val));
      }
    else // Slice Number present but empty
      {
      (*it).second.SliceNumber = 0;
      }
    }

  // cache the slice number
  if( val )
    {
    this->SliceNumber = atoi(reinterpret_cast<char *>(val));
    }
  else // Slice Number present but empty
    {
    this->SliceNumber = 0;
    }
}


void DICOMAppHelper::SliceLocationCallback(DICOMParser *parser,
                                           doublebyte,
                                           doublebyte,
                                           DICOMParser::VRTypes,
                                           unsigned char* val,
                                           quadbyte)
{
  // Look for the current file in the map of slice ordering data
  dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator it;
  it = this->Implementation->SliceOrderingMap.find(parser->GetFileName());
  if (it == Implementation->SliceOrderingMap.end())
    {
    // file not found, create a new entry
    DICOMOrderingElements ord;
    ord.SliceLocation = static_cast<float>(
      atof(reinterpret_cast<char *>(val)));

    // insert into the map
    this->Implementation->SliceOrderingMap.insert(dicom_stl::pair<const dicom_stl::string,
      DICOMOrderingElements>(parser->GetFileName(), ord));
    }
  else if (val)
    {
    // file found, add new values
    (*it).second.SliceLocation =
      static_cast<float>(atof(reinterpret_cast<char *>(val) ));
    }
}

void DICOMAppHelper::ImagePositionPatientCallback(DICOMParser *parser,
                                                  doublebyte,
                                                  doublebyte,
                                                  DICOMParser::VRTypes,
                                                  unsigned char* val,
                                                  quadbyte)
{
  // Look for the current file in the map of slice ordering data
  dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator it;
  it = this->Implementation->SliceOrderingMap.find(parser->GetFileName());
  if (it == Implementation->SliceOrderingMap.end())
    {
    // file not found, create a new entry
    DICOMOrderingElements ord;

    if (val)
      {
      sscanf(reinterpret_cast<char*>(val), "%f\\%f\\%f",
              &ord.ImagePositionPatient[0],
              &ord.ImagePositionPatient[1],
              &ord.ImagePositionPatient[2] );
      }
    else
      {
      // no actual position specified, default to the origin
      ord.ImagePositionPatient[0] = 0.0;
      ord.ImagePositionPatient[1] = 0.0;
      ord.ImagePositionPatient[2] = 0.0;
      }

    // insert into the map
    this->Implementation->SliceOrderingMap.insert(dicom_stl::pair<const dicom_stl::string,
      DICOMOrderingElements>(parser->GetFileName(), ord));

    // cache the value
    memcpy( this->ImagePositionPatient, ord.ImagePositionPatient,
            3*sizeof(float) );
    }
  else
    {
    if (val)
      {
      // file found, add new values
      sscanf( reinterpret_cast<char*>(val), "%f\\%f\\%f",
              &(*it).second.ImagePositionPatient[0],
              &(*it).second.ImagePositionPatient[1],
              &(*it).second.ImagePositionPatient[2] );
      }
    else
      {
      // no actual position specified, default to the origin
      (*it).second.ImagePositionPatient[0] = 0.0;
      (*it).second.ImagePositionPatient[1] = 0.0;
      (*it).second.ImagePositionPatient[2] = 0.0;
      }

    // cache the value
    memcpy( this->ImagePositionPatient, (*it).second.ImagePositionPatient,
            3*sizeof(float) );
    }
}


void DICOMAppHelper::ImageOrientationPatientCallback(DICOMParser *parser,
                                                     doublebyte,
                                                     doublebyte,
                                                     DICOMParser::VRTypes,
                                                     unsigned char* val,
                                                     quadbyte)
{
  // Look for the current file in the map of slice ordering data
  dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator it;
  it = this->Implementation->SliceOrderingMap.find(parser->GetFileName());
  if (it == Implementation->SliceOrderingMap.end())
    {
    // file not found, create a new entry
    DICOMOrderingElements ord;
    if (val)
      {
      sscanf( reinterpret_cast<char*>(val), "%f\\%f\\%f\\%f\\%f\\%f",
              &ord.ImageOrientationPatient[0],
              &ord.ImageOrientationPatient[1],
              &ord.ImageOrientationPatient[2],
              &ord.ImageOrientationPatient[3],
              &ord.ImageOrientationPatient[4],
              &ord.ImageOrientationPatient[5] );
      }
    else
      {
      // no orientation defined, default to an standard axial orientation
      ord.ImageOrientationPatient[0] = 1.0;
      ord.ImageOrientationPatient[1] = 0.0;
      ord.ImageOrientationPatient[2] = 0.0;
      ord.ImageOrientationPatient[3] = 0.0;
      ord.ImageOrientationPatient[4] = 1.0;
      ord.ImageOrientationPatient[5] = 0.0;
      }

    // insert into the map
    this->Implementation->SliceOrderingMap.insert(dicom_stl::pair<const dicom_stl::string,
      DICOMOrderingElements>(parser->GetFileName(), ord));

    // cache the value
    memcpy( this->ImageOrientationPatient, ord.ImageOrientationPatient,
            6*sizeof(float) );
    }
  else
    {
    // file found, add new values
    if (val)
      {
      sscanf( reinterpret_cast<char*>(val), "%f\\%f\\%f\\%f\\%f\\%f",
              &(*it).second.ImageOrientationPatient[0],
              &(*it).second.ImageOrientationPatient[1],
              &(*it).second.ImageOrientationPatient[2],
              &(*it).second.ImageOrientationPatient[3],
              &(*it).second.ImageOrientationPatient[4],
              &(*it).second.ImageOrientationPatient[5] );
      }
    else
      {
      // no orientation defined, default to an standard axial orientation
      (*it).second.ImageOrientationPatient[0] = 1.0;
      (*it).second.ImageOrientationPatient[1] = 0.0;
      (*it).second.ImageOrientationPatient[2] = 0.0;
      (*it).second.ImageOrientationPatient[3] = 0.0;
      (*it).second.ImageOrientationPatient[4] = 1.0;
      (*it).second.ImageOrientationPatient[5] = 0.0;
      }

    // cache the value
    memcpy( this->ImageOrientationPatient, (*it).second.ImageOrientationPatient,
            6*sizeof(float) );
    }
}


void DICOMAppHelper::TransferSyntaxCallback(DICOMParser *parser,
                                            doublebyte,
                                            doublebyte,
                                            DICOMParser::VRTypes,
                                            unsigned char* val,
                                            quadbyte)
{

#ifdef DEBUG_DICOM_APP_HELPER
#ifdef _WIN32
  char platformByteOrder = 'L';
#else
  char platformByteOrder = 'B';
#endif
  dicom_stream::cout << "Platform byte order: " << platformByteOrder << dicom_stream::endl;
#endif

  static const char* TRANSFER_UID_EXPLICIT_BIG_ENDIAN = "1.2.840.10008.1.2.2";

  // Only add the ToggleSwapBytes callback when we need it.
  if (strcmp(TRANSFER_UID_EXPLICIT_BIG_ENDIAN,
             reinterpret_cast<char*>(val)) == 0)
    {
    this->ByteSwapData = true;
    parser->AddDICOMTagCallback(0x0800, 0x0000, DICOMParser::VR_UNKNOWN, ToggleSwapBytesCB);
#ifdef DEBUG_DICOM_APP_HELPER
    dicom_stream::cerr <<"Registering callback for swapping bytes." << dicom_stream::endl;
#endif
    }

  delete this->TransferSyntaxUID;
  this->TransferSyntaxUID = new dicom_stl::string(
    reinterpret_cast<char*>(val));

#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Transfer Syntax UID: " << *this->TransferSyntaxUID;
  dicom_stream::cout << " " << this->TransferSyntaxUIDDescription(this->TransferSyntaxUID->c_str()) << dicom_stream::endl;
#endif
}

void DICOMAppHelper::BitsAllocatedCallback(DICOMParser *parser,
                                           doublebyte,
                                           doublebyte,
                                           DICOMParser::VRTypes,
                                           unsigned char* val,
                                           quadbyte)
{
  this->BitsAllocated = parser->GetDICOMFile()->ReturnAsUnsignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Bits allocated: " << this->BitsAllocated << dicom_stream::endl;
#endif
}


void DICOMAppHelper::ToggleSwapBytesCallback(DICOMParser *parser,
                                             doublebyte,
                                             doublebyte,
                                             DICOMParser::VRTypes,
                                             unsigned char* ,
                                             quadbyte len)
{
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "ToggleSwapBytesCallback" << dicom_stream::endl;
#endif
  bool bs = parser->GetDICOMFile()->GetPlatformIsBigEndian();
  parser->GetDICOMFile()->SetPlatformIsBigEndian(!bs);

#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Set byte swap to: " << parser->GetDICOMFile()->GetPlatformIsBigEndian() << dicom_stream::endl;
#endif

  long pos = parser->GetDICOMFile()->Tell();

  //
  // The +4 is probably a hack, but it's a guess at the length of the previous field.
  //
  parser->GetDICOMFile()->SkipToPos(pos - len + 4);
}


//
// 0028,0030 is Pixel Spacing, which is NOT the pixel spacing for modality such as US, or X-ray
// see Imager Pixel Spacing and Pixel Ratio instead
//
void DICOMAppHelper::PixelSpacingCallback(DICOMParser *parser,
                                          doublebyte group,
                                          doublebyte element,
                                          DICOMParser::VRTypes,
                                          unsigned char* val,
                                          quadbyte)
{
  if (group == 0x0028 && element == 0x0030)
    {
    if (!val || sscanf(reinterpret_cast<char*>(val), "%f\\%f",
                       &this->PixelSpacing[0],
                       &this->PixelSpacing[1]) != 2)
      {
      this->PixelSpacing[0] = this->PixelSpacing[1] = 0.0;
      }
    }
  else if (group == 0x0018 && element == 0x0050)
    {
    this->PixelSpacing[2] =
      DICOMFile::ReturnAsFloat(
        val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
    }
}

void DICOMAppHelper::WidthCallback(DICOMParser *parser,
                                   doublebyte,
                                   doublebyte,
                                   DICOMParser::VRTypes,
                                   unsigned char* val,
                                   quadbyte)
{
  unsigned short uival = DICOMFile::ReturnAsUnsignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Width: " << uival << dicom_stream::endl;
#endif

  this->Width = uival;
  this->Dimensions[0] = this->Width;
}

void DICOMAppHelper::HeightCallback(DICOMParser *parser,
                                    doublebyte,
                                    doublebyte,
                                    DICOMParser::VRTypes,
                                    unsigned char* val,
                                    quadbyte)
{
  unsigned short uival = DICOMFile::ReturnAsUnsignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Height: " << uival << dicom_stream::endl;
#endif
  this->Height = uival;
  this->Dimensions[1] = this->Height;
}


void DICOMAppHelper::PixelRepresentationCallback( DICOMParser *parser,
                                                  doublebyte,
                                                  doublebyte,
                                                  DICOMParser::VRTypes,
                                                  unsigned char* val,
                                                  quadbyte)
{
  unsigned short uival = DICOMFile::ReturnAsUnsignedShort(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Pixel Representation: " << (uival ? "Signed" : "Unsigned") << dicom_stream::endl;
#endif
  this->PixelRepresentation = uival;
}

void DICOMAppHelper::PhotometricInterpretationCallback( DICOMParser *,
                                                        doublebyte,
                                                        doublebyte,
                                                        DICOMParser::VRTypes,
                                                        unsigned char* val,
                                                        quadbyte)
{
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Photometric Interpretation: " << (char*) val << dicom_stream::endl;
#endif
  delete this->PhotometricInterpretation;

  this->PhotometricInterpretation = new dicom_stl::string(
    reinterpret_cast<char*>(val));
}

void DICOMAppHelper::PixelDataCallback( DICOMParser *,
                                        doublebyte,
                                        doublebyte,
                                        DICOMParser::VRTypes,
                                        unsigned char* data,
                                        quadbyte len)
{
  int numPixels = this->Dimensions[0] * this->Dimensions[1] * this->GetNumberOfComponents();
  if (len < numPixels)
    {
    numPixels = len;
    }
  if (numPixels < 0)
    {
    numPixels = 0;
    }

#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "numPixels : " << numPixels << dicom_stream::endl;
#endif

  int ptrIncr = int(this->BitsAllocated/8.0);

  unsigned short* ushortInputData = reinterpret_cast<unsigned short*>(data);
  unsigned char* ucharInputData = data;
  short* shortInputData = reinterpret_cast<short*> (data);

  float* floatOutputData; // = NULL;

  bool isFloat = this->RescaledImageDataIsFloat();

  if (isFloat)
    {
#ifdef DEBUG_DICOM_APP_HELPER
    dicom_stream::cout << "Slope and offset are not integer valued : ";
    dicom_stream::cout << this->RescaleSlope << ", " << this->RescaleOffset << dicom_stream::endl;
#endif
    delete [] (static_cast<char*> (this->ImageData));
    this->ImageData = new float[numPixels];
    floatOutputData = static_cast<float*> (this->ImageData);

    this->ImageDataType = DICOMParser::VR_FL;
    unsigned long uNumPixels=static_cast<unsigned long>(numPixels);
    this->ImageDataLengthInBytes = uNumPixels*sizeof(float);
    float newFloatPixel;

    if (ptrIncr == 1)
      {
      for (int i = 0; i < numPixels; i++)
        {
        newFloatPixel = float(static_cast<double>(this->RescaleSlope) * ucharInputData[i] + static_cast<double>(this->RescaleOffset));
        floatOutputData[i] = newFloatPixel;
        }
#ifdef DEBUG_DICOM_APP_HELPER
      dicom_stream::cout << "Did rescale, offset to float from char." << dicom_stream::endl;
      dicom_stream::cout << numPixels << " pixels." << dicom_stream::endl;
#endif
      }
    else if (ptrIncr == 2)
      {
      for (int i = 0; i < numPixels; i++)
        {
        newFloatPixel = float(static_cast<double>(this->RescaleSlope) * ushortInputData[i] + static_cast<double>(this->RescaleOffset));
        floatOutputData[i] = newFloatPixel;
        }
#ifdef DEBUG_DICOM_APP_HELPER
      dicom_stream::cout << "Did rescale, offset to float from short." << dicom_stream::endl;
      dicom_stream::cout << numPixels << " pixels." << dicom_stream::endl;
#endif
      }
    }
  else
    {
#ifdef DEBUG_DICOM_APP_HELPER
    dicom_stream::cout << "Slope and offset are integer valued : ";
    dicom_stream::cout << this->RescaleSlope << ", " << this->RescaleOffset << dicom_stream::endl;
#endif

    if (ptrIncr == 1)
      {
      delete [] (static_cast<char*> (this->ImageData));
      this->ImageData = new char[numPixels];

      char*  charOutputData =  static_cast<char*>  (this->ImageData);

      this->ImageDataType = DICOMParser::VR_OB;
      unsigned long uNumPixels=static_cast<unsigned long>(numPixels);
      this->ImageDataLengthInBytes = uNumPixels * sizeof(char);
      char newCharPixel;

      for (int i = 0; i < numPixels; i++)
        {
        newCharPixel = char(static_cast<double>(this->RescaleSlope) * ucharInputData[i] + static_cast<double>(this->RescaleOffset));
        charOutputData[i] = newCharPixel;
        }
#ifdef DEBUG_DICOM_APP_HELPER
      dicom_stream::cout << "Did rescale, offset to char from char." << dicom_stream::endl;
      dicom_stream::cout << numPixels << " pixels." << dicom_stream::endl;
#endif
      }
    else if (ptrIncr == 2)
      {
      delete [] (static_cast<char*> (this->ImageData));
      this->ImageData = new short[numPixels];
      short* shortOutputData = static_cast<short*> (this->ImageData);

      this->ImageDataType = DICOMParser::VR_OW;
      unsigned long uNumPixels=static_cast<unsigned long>(numPixels);
      this->ImageDataLengthInBytes = uNumPixels * sizeof(short);
      short newShortPixel;
      for (int i = 0; i < numPixels; i++)
        {
        newShortPixel = short(static_cast<double>(this->RescaleSlope) * shortInputData[i] + static_cast<double>(this->RescaleOffset));
        shortOutputData[i] = newShortPixel;
        }
#ifdef DEBUG_DICOM_APP_HELPER
      dicom_stream::cout << "Did rescale, offset to short from short." << dicom_stream::endl;
      dicom_stream::cout << numPixels << " pixels." << dicom_stream::endl;
#endif
      }
    }
}

void DICOMAppHelper::RegisterPixelDataCallback(DICOMParser* parser)
{
  this->PixelDataCB->SetCallbackFunction(this, &DICOMAppHelper::PixelDataCallback);
  parser->AddDICOMTagCallback(0x7FE0, 0x0010, DICOMParser::VR_OW, this->PixelDataCB);
}

void DICOMAppHelper::RescaleOffsetCallback( DICOMParser *parser,
                                            doublebyte,
                                            doublebyte,
                                            DICOMParser::VRTypes,
                                            unsigned char* val,
                                            quadbyte)
{
  float fval = DICOMFile::ReturnAsFloat(val, parser->GetDICOMFile()->GetPlatformIsBigEndian());
  this->RescaleOffset = fval;
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Pixel offset: " << this->RescaleOffset << dicom_stream::endl;
#endif
}

const char* DICOMAppHelper::TransferSyntaxUIDDescription(const char* uid)
{
  static const char* DICOM_IMPLICIT_VR_LITTLE_ENDIAN = "1.2.840.10008.1.2";
  static const char* DICOM_LOSSLESS_JPEG = "1.2.840.10008.1.2.4.70";
  static const char* DICOM_LOSSY_JPEG_8BIT = "1.2.840.10008.1.2.4.50";
  static const char* DICOM_LOSSY_JPEG_16BIT = "1.2.840.10008.1.2.4.51";
  static const char* DICOM_EXPLICIT_VR_LITTLE_ENDIAN = "1.2.840.10008.1.2.1";
  static const char* DICOM_EXPLICIT_VR_BIG_ENDIAN = "1.2.840.10008.1.2.2";
  static const char* DICOM_GE_PRIVATE_IMPLICIT_BIG_ENDIAN = "1.2.840.113619.5.2";

  if (!strcmp(DICOM_IMPLICIT_VR_LITTLE_ENDIAN, uid))
    {
    return "Implicit VR, Little Endian";
    }
  else if (!strcmp(DICOM_LOSSLESS_JPEG, uid))
    {
    return "Lossless JPEG";
    }
  else if (!strcmp(DICOM_LOSSY_JPEG_8BIT, uid))
    {
    return "Lossy JPEG 8 bit";
    }
  else if (!strcmp(DICOM_LOSSY_JPEG_16BIT, uid))
    {
    return "Lossy JPEG 16 bit.";
    }
  else if (!strcmp(DICOM_EXPLICIT_VR_LITTLE_ENDIAN, uid))
    {
    return "Explicit VR, Little Endian.";
    }
  else if (!strcmp(DICOM_EXPLICIT_VR_BIG_ENDIAN, uid))
    {
    return "Explicit VR, Big Endian.";
    }
  else if (!strcmp(DICOM_GE_PRIVATE_IMPLICIT_BIG_ENDIAN, uid))
    {
    return "GE Private, Implicit VR, Big Endian Image Data.";
    }
  else
    {
    return "Unknown.";
    }

}


void DICOMAppHelper::RescaleSlopeCallback(DICOMParser *parser,
                                          doublebyte,
                                          doublebyte ,
                                          DICOMParser::VRTypes ,
                                          unsigned char* val,
                                          quadbyte )
{
  float fval = DICOMFile::ReturnAsFloat(val,
                                        parser->GetDICOMFile()->GetPlatformIsBigEndian ());
#ifdef DEBUG_DICOM_APP_HELPER
  dicom_stream::cout << "Rescale slope: " << fval << dicom_stream::endl;
#endif
  this->RescaleSlope = fval;
}

bool DICOMAppHelper::RescaledImageDataIsFloat()
{
  int s = int(this->RescaleSlope);
  int o = int(this->RescaleOffset);

  float sf = float(s);
  float of = float(o);

  double d1 = fabs(sf - this->RescaleSlope);
  double d2 = fabs(of - this->RescaleOffset);

  if (d1 > 0.0 || d2 > 0.0)
    {
    return true;
    }
  else
    {
    return false;
    }
}

void DICOMAppHelper::GetImageData(void*& data, DICOMParser::VRTypes& dataType, unsigned long& len)
{
  data = this->ImageData;
  dataType = this->ImageDataType;
  len = this->ImageDataLengthInBytes;
}

bool DICOMAppHelper::RescaledImageDataIsSigned()
{
  bool rescaleSigned = (this->RescaleSlope < 0.0);
  bool pixelRepSigned = (this->PixelRepresentation == 1);
  bool offsetSigned = (this->RescaleOffset < 0.0);

  return (rescaleSigned || pixelRepSigned || offsetSigned);
}


void DICOMAppHelper::GetSliceNumberFilenamePairs(const dicom_stl::string &seriesUID,
                                                 dicom_stl::vector<dicom_stl::pair<int, dicom_stl::string> >& v, bool ascending)
{
  v.clear();

  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr >::iterator miter  = this->Implementation->SeriesUIDMap.find(seriesUID);

  if (miter == this->Implementation->SeriesUIDMap.end() )
    {
    return;
    }

  // grab the filenames for the specified series
  dicom_stl::vector<dicom_stl::string> files = (*miter).second;

  for (dicom_stl::vector<dicom_stl::string>::iterator fileIter = files.begin();
       fileIter != files.end();
       fileIter++)
       {
       dicom_stl::pair<int, dicom_stl::string> p;
       p.second = dicom_stl::string(*fileIter);
       int slice_number;
       dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator sn_iter = Implementation->SliceOrderingMap.find(*fileIter);
       // Only store files that have a valid slice number
       if (sn_iter != Implementation->SliceOrderingMap.end())
        {
        slice_number = (*sn_iter).second.SliceNumber;
        p.first = slice_number;
        v.push_back(p);
        }
       }
  if (ascending)
    {
    dicom_stl::sort(v.begin(), v.end(), lt_pair_int_string());
    }
  else
    {
    dicom_stl::sort(v.begin(), v.end(), gt_pair_int_string());
    }
}

void DICOMAppHelper::GetSliceNumberFilenamePairs(dicom_stl::vector<dicom_stl::pair<int, dicom_stl::string> >& v, bool ascending)
{
  // Default to using the first series
  if (!this->Implementation->SeriesUIDMap.empty())
    {
    this->GetSliceNumberFilenamePairs( (*this->Implementation->SeriesUIDMap.begin()).first, v, ascending );
    }
  else
    {
    v.clear();
    }
}

void DICOMAppHelper::GetSliceLocationFilenamePairs(const dicom_stl::string &seriesUID,
                                                   dicom_stl::vector<dicom_stl::pair<float, dicom_stl::string> >& v, bool ascending)
{
  v.clear();

  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr >::iterator miter  = this->Implementation->SeriesUIDMap.find(seriesUID);

  if (miter == this->Implementation->SeriesUIDMap.end() )
    {
    return;
    }

  // grab the filenames for the specified series
  dicom_stl::vector<dicom_stl::string> files = (*miter).second;

  for (dicom_stl::vector<dicom_stl::string>::iterator fileIter = files.begin();
       fileIter != files.end();
       fileIter++)
       {
       dicom_stl::pair<float, dicom_stl::string> p;
       p.second = dicom_stl::string(*fileIter);
       float slice_location;
       dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator sn_iter = Implementation->SliceOrderingMap.find(*fileIter);

       if (sn_iter != Implementation->SliceOrderingMap.end())
        {
        slice_location = (*sn_iter).second.SliceLocation;
        p.first = slice_location;
        v.push_back(p);
        }
       }
  if (ascending)
    {
    dicom_stl::sort(v.begin(), v.end(), lt_pair_float_string());
    }
  else
    {
    dicom_stl::sort(v.begin(), v.end(), gt_pair_float_string());
    }
}

void DICOMAppHelper::GetSliceLocationFilenamePairs(dicom_stl::vector<dicom_stl::pair<float, dicom_stl::string> >& v, bool ascending)
{
  // Default to using the first series
  if (!this->Implementation->SeriesUIDMap.empty())
    {
    this->GetSliceLocationFilenamePairs( (*this->Implementation->SeriesUIDMap.begin()).first,
                                         v , ascending);
    }
  else
    {
    v.clear();
    }
}

void DICOMAppHelper::GetImagePositionPatientFilenamePairs(const dicom_stl::string &seriesUID, dicom_stl::vector<dicom_stl::pair<float, dicom_stl::string> >& v, bool ascending)
{
  v.clear();

  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr >::iterator miter  = this->Implementation->SeriesUIDMap.find(seriesUID);

  if (miter == this->Implementation->SeriesUIDMap.end() )
    {
    return;
    }

  // grab the filenames for the specified series
  dicom_stl::vector<dicom_stl::string> files = (*miter).second;

  for (dicom_stl::vector<dicom_stl::string>::iterator fileIter = files.begin();
       fileIter != files.end();
       fileIter++)
       {
       dicom_stl::pair<float, dicom_stl::string> p;
       p.second = dicom_stl::string(*fileIter);

       float image_position;
       float normal[3];

       dicom_stl::map<dicom_stl::string, DICOMOrderingElements, ltstdstr>::iterator sn_iter =
         Implementation->SliceOrderingMap.find(*fileIter);

       if (sn_iter != Implementation->SliceOrderingMap.end())
        {
        // compute the image patient position wrt to the slice image
        // plane normal

        normal[0] = ((*sn_iter).second.ImageOrientationPatient[1]
                     * (*sn_iter).second.ImageOrientationPatient[5])
          - ((*sn_iter).second.ImageOrientationPatient[2]
             * (*sn_iter).second.ImageOrientationPatient[4]);
        normal[1] = ((*sn_iter).second.ImageOrientationPatient[2]
                     *(*sn_iter).second.ImageOrientationPatient[3])
          - ((*sn_iter).second.ImageOrientationPatient[0]
             * (*sn_iter).second.ImageOrientationPatient[5]);
        normal[2] = ((*sn_iter).second.ImageOrientationPatient[0]
                     * (*sn_iter).second.ImageOrientationPatient[4])
          - ((*sn_iter).second.ImageOrientationPatient[1]
             * (*sn_iter).second.ImageOrientationPatient[3]);

        image_position = (normal[0]*(*sn_iter).second.ImagePositionPatient[0])
          + (normal[1]*(*sn_iter).second.ImagePositionPatient[1])
          + (normal[2]*(*sn_iter).second.ImagePositionPatient[2]);
        p.first = image_position;
        v.push_back(p);
        }
       }
  if (ascending)
    {
    dicom_stl::sort(v.begin(), v.end(), lt_pair_float_string());
    }
  else
    {
    dicom_stl::sort(v.begin(), v.end(), gt_pair_float_string());
    }
}


void DICOMAppHelper::GetImagePositionPatientFilenamePairs(dicom_stl::vector<dicom_stl::pair<float, dicom_stl::string> >& v, bool ascending)
{
  // Default to using the first series
  if (!this->Implementation->SeriesUIDMap.empty())
    {
    this->GetImagePositionPatientFilenamePairs(
      (*this->Implementation->SeriesUIDMap.begin()).first, v, ascending);
    }
  else
    {
    v.clear();
    }
}

void DICOMAppHelper::GetSeriesUIDs(dicom_stl::vector<dicom_stl::string> &v)
{
  v.clear();

  dicom_stl::map<dicom_stl::string, dicom_stl::vector<dicom_stl::string>, ltstdstr >::iterator miter;

  for (miter = this->Implementation->SeriesUIDMap.begin(); miter != this->Implementation->SeriesUIDMap.end();
       ++miter)
    {
    v.push_back( (*miter).first );
    }
}

void DICOMAppHelper::Clear()
{
  this->Implementation->SliceOrderingMap.clear();
  this->Implementation->SeriesUIDMap.clear();
}

void DICOMAppHelper::PatientNameCallback(DICOMParser *,
                                         doublebyte,
                                         doublebyte,
                                         DICOMParser::VRTypes,
                                         unsigned char* val,
                                         quadbyte)
{
  delete this->PatientName;

  if (val)
    {
    this->PatientName = new dicom_stl::string(reinterpret_cast<char*>(val));
    }
  else
    {
    this->PatientName = new dicom_stl::string();
    }
}

void DICOMAppHelper::StudyUIDCallback(DICOMParser *,
                                         doublebyte,
                                         doublebyte,
                                         DICOMParser::VRTypes,
                                         unsigned char* val,
                                         quadbyte)
{
  delete this->StudyUID;

  this->StudyUID = new dicom_stl::string(reinterpret_cast<char*>(val));

}

void DICOMAppHelper::StudyIDCallback(DICOMParser *,
                                         doublebyte,
                                         doublebyte,
                                         DICOMParser::VRTypes,
                                         unsigned char* val,
                                         quadbyte)
{
  delete this->StudyID;

  if (val)
    {
    this->StudyID = new dicom_stl::string(reinterpret_cast<char*>(val));
    }
  else
    {
    this->StudyID = new dicom_stl::string();
    }

}

void DICOMAppHelper::GantryAngleCallback(DICOMParser * parser,
                           doublebyte,
                           doublebyte,
                           DICOMParser::VRTypes,
                           unsigned char* val,
                           quadbyte)
{
  float fval = DICOMFile::ReturnAsFloat(val,
                                        parser->GetDICOMFile()->GetPlatformIsBigEndian ());

  this->GantryAngle = fval;
}


#ifdef _MSC_VER
#pragma warning ( pop )
#endif

