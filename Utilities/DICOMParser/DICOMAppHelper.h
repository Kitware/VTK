
#ifndef __DICOM_APP_HELPER_H_
#define __DICOM_APP_HELPER_H_

#include <fstream>
#include "DICOMTypes.h"
#include "DICOMParser.h"
#include "DICOMCallback.h"

#include <vector>
#include <string>
#include <iomanip>
#include <iostream>

struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

struct ltstdstr
{
  bool operator()(const std::string s1, const std::string s2) const
  {
    return s1 < s2;
  }
};

struct DICOMTagInfo
{
  doublebyte group;
  doublebyte element;
  DICOMParser::VRTypes datatype;
  char* description;
};

class DICOMAppHelper
{
 public:
  DICOMAppHelper();
  virtual ~DICOMAppHelper();
  
  void SetFileName(const char* filename);

  virtual void RescaleSlopeCallback(doublebyte group,
                     doublebyte element,
                     DICOMParser::VRTypes type,
                     unsigned char* val,
                     quadbyte len);


  virtual void ArrayCallback(doublebyte group,
                     doublebyte element,
                     DICOMParser::VRTypes type,
                     unsigned char* val,
                     quadbyte len);
  
  virtual void SliceNumberCallback(doublebyte group,
                           doublebyte element,
                           DICOMParser::VRTypes type,
                           unsigned char* val,
                           quadbyte len) ;
  
  virtual void SeriesUIDCallback(doublebyte group,
                         doublebyte element,
                         DICOMParser::VRTypes type,
                         unsigned char* val,
                         quadbyte len) ;
    
  virtual void TransferSyntaxCallback(doublebyte group,
                              doublebyte element,
                              DICOMParser::VRTypes type,
                              unsigned char* val,
                              quadbyte len) ;
  
  virtual void BitsAllocatedCallback(doublebyte group,
                             doublebyte element,
                             DICOMParser::VRTypes type,
                             unsigned char* val,
                             quadbyte len) ;
  
  void OutputSeries();

  virtual void RegisterCallbacks(DICOMParser* parser);

  virtual void SetDICOMDataFile(DICOMFile* f)
    {
    this->DICOMDataFile = f;
    }

  virtual void ToggleSwapBytesCallback(doublebyte,
                               doublebyte,
                               DICOMParser::VRTypes,
                               unsigned char*,
                               quadbyte);

  virtual void PixelSpacingCallback(doublebyte group,
                                    doublebyte element,
                                    DICOMParser::VRTypes type,
                                    unsigned char* val,
                                    quadbyte len) ;

  virtual void HeightCallback(doublebyte group,
                              doublebyte element,
                              DICOMParser::VRTypes type,
                              unsigned char* val,
                              quadbyte len);

  virtual void WidthCallback( doublebyte group,
                              doublebyte element,
                              DICOMParser::VRTypes type,
                              unsigned char* val,
                              quadbyte len);

  virtual void PixelRepresentationCallback(doublebyte group,
                                           doublebyte element,
                                           DICOMParser::VRTypes type,
                                           unsigned char* val,
                                           quadbyte len);

  virtual void PhotometricInterpretationCallback(doublebyte,
                                                 doublebyte,
                                                 DICOMParser::VRTypes,
                                                 unsigned char* val,
                                                 quadbyte len);

  virtual void PixelDataCallback(doublebyte,
                                 doublebyte,
                                 DICOMParser::VRTypes,
                                 unsigned char* val,
                                 quadbyte len);

  virtual void RescaleOffsetCallback( doublebyte,
                                    doublebyte,
                                    DICOMParser::VRTypes,
                                    unsigned char* val,
                                    quadbyte);

  float* GetPixelSpacing()
    {
    return this->PixelSpacing;
    }

  int GetWidth()
    {
    return this->Width;
    }

  int GetHeight()
    {
    return this->Height;
    }

  int* GetDimensions()
    {
    return this->Dimensions;
    }

  int GetBitsAllocated()
    {
    return this->BitsAllocated;
    }

  // 0 unsigned
  // 1 signed
  int GetPixelRepresentation()
    {
    return this->PixelRepresentation;
    }

  int GetNumberOfComponents()
    {
    if (!this->PhotometricInterpretation)
      {
      return 1;
      }

    //
    // DICOM standard says that spaces (0x20) are to
    // be ignored for CS types.  We don't handle this
    // well yet.
    //
    std::string str1(*this->PhotometricInterpretation);
    std::string rgb("RGB ");

    if (str1 == rgb)
      {
      return 3;
      }
    else
      {
      return 1;
      }
    }


  virtual void RegisterPixelDataCallback();

  std::string GetTransferSyntaxUID()
    {
    return *(this->TransferSyntaxUID);
    }

  const char* TransferSyntaxUIDDescription(const char* uid);


  void GetImageData(void* & data, DICOMParser::VRTypes& dataType, unsigned long& len);

  bool RescaledImageDataIsFloat();

  bool RescaledImageDataIsSigned();

  int GetSliceNumber()
    {
    return this->SliceNumber;
    }

  void ClearSliceNumberMap();

  void ClearSeriesUIDMap();

  void GetSliceNumberFilenamePairs(std::vector<std::pair<int, std::string> > & v);

 protected:
  int BitsAllocated;
  bool ByteSwapData;
  float PixelSpacing[3];
  int Width;
  int Height;
  int SliceNumber; 
  int Dimensions[2];

  char* GetOutputFilename()
  {
    int len = static_cast<int>(strlen(this->FileName));
    char* output = new char[len + 5];
    strcpy(output, this->FileName);
    strcat(output, ".raw");
    return output;
  }
  
  char* FileName;
  
  // map from series UID to vector of files in the series 
  // std::map<char*, std::vector<char*>, ltstr > SeriesUIDMap;  
  // std::map<char*, int, ltstr> SliceNumberMap;  
  std::map<std::string, std::vector<std::string>, ltstdstr> SeriesUIDMap;
  std::map<std::string, int, ltstdstr> SliceNumberMap;

  typedef std::map<std::pair<doublebyte, doublebyte>, DICOMTagInfo> TagMapType;
  TagMapType TagMap;

  std::ofstream HeaderFile;
  
  DICOMFile* DICOMDataFile;

  DICOMParser* Parser;

  // 0 unsigned
  // 1 2s complement (signed)
  int PixelRepresentation;
  std::string* PhotometricInterpretation;
  std::string* TransferSyntaxUID;
  float RescaleOffset;
  float RescaleSlope;
  void* ImageData;
  DICOMParser::VRTypes ImageDataType;
  unsigned long ImageDataLengthInBytes;

  DICOMMemberCallback<DICOMAppHelper>* SeriesUIDCB;
  DICOMMemberCallback<DICOMAppHelper>* SliceNumberCB;
  DICOMMemberCallback<DICOMAppHelper>* TransferSyntaxCB;
  DICOMMemberCallback<DICOMAppHelper>* BitsAllocatedCB;
  DICOMMemberCallback<DICOMAppHelper>* PixelSpacingCB;
  DICOMMemberCallback<DICOMAppHelper>* HeightCB;
  DICOMMemberCallback<DICOMAppHelper>* WidthCB;
  DICOMMemberCallback<DICOMAppHelper>* PixelRepresentationCB;
  DICOMMemberCallback<DICOMAppHelper>* PhotometricInterpretationCB;
  DICOMMemberCallback<DICOMAppHelper>* RescaleOffsetCB;
  DICOMMemberCallback<DICOMAppHelper>* RescaleSlopeCB;
  DICOMMemberCallback<DICOMAppHelper>* PixelDataCB;

};

#endif
