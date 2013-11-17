/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMParserMap.h
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

#ifndef __DICOM_PARSER_MAP__H_
#define __DICOM_PARSER_MAP__H_

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

#include "DICOMConfig.h"

class DICOMCallback;

//
// Structure that implements a compare operator for
// a pair of doublebytes.  This is used when comparing
// group, element pairs.
//
struct group_element_compare
{
  bool operator() (const dicom_stl::pair<doublebyte, doublebyte> p1, const dicom_stl::pair<doublebyte, doublebyte> p2) const
  {
    if (p1.first < p2.first)
      {
      return true;
      }
    else if (p1.first == p2.first)
      {
      if (p1.second < p2.second)
        {
        return true;
        }
      else
        {
        return false;
        }
      }
    else
      {
      return false;
      }
  }
};

//
// Typedef a pair of doublebytes
//
typedef dicom_stl::pair<doublebyte, doublebyte> DICOMMapKeyOverride;

// DICOM_EXPIMP_TEMPLATE template struct DICOM_MAP_EXPORT dicom_stream::pair<doublebyte, doublebyte>;
//
// Subclass of a pair of doublebytes to make
// type names shorter in the code.
//
class  DICOMMapKey : public DICOMMapKeyOverride
{
 public:
  DICOMMapKey(doublebyte v1, doublebyte v2) :
    dicom_stl::pair<doublebyte, doublebyte> (v1, v2)
  {

  }

};

//
// Typedef of a pair of doublebyte, vector.
//
typedef dicom_stl::pair<doublebyte, dicom_stl::vector<DICOMCallback*>*> DICOMMapValueOverride;


// DICOM_EXPIMP_TEMPLATE template struct DICOM_MAP_EXPORT dicom_stream::pair<doublebyte, dicom_stream::vector<DICOMCallback*>*>;

//
// Subclass of pair doublebyte, vector<DICOMCallback*>.
// Makes types shorter in the code.
//
class  DICOMMapValue : public DICOMMapValueOverride
{
 public:
   DICOMMapValue() : dicom_stl::pair<doublebyte, dicom_stl::vector<DICOMCallback*>*>() {}

  DICOMMapValue(doublebyte v1, dicom_stl::vector<DICOMCallback*> * v2) :
    dicom_stl::pair<doublebyte, dicom_stl::vector<DICOMCallback*>*>(v1, v2)
  {

  }
};


// DICOM_EXPIMP_TEMPLATE template class DICOM_MAP_EXPORT dicom_stream::map<DICOMMapKey, DICOMMapValue, group_element_compare>;

//
// Subclass of the particular map we're using.  Again,
// makes type specifiers shorter in the code.
//
class  DICOMParserMap :
  public  dicom_stl::map<DICOMMapKey, DICOMMapValue, group_element_compare>
{

};

typedef doublebyte DICOMTypeValue;

// DICOM_EXPIMP_TEMPLATE template class  dicom_stream::map<DICOMMapKey, DICOMTypeValue, group_element_compare>;

class  DICOMImplicitTypeMap :
  public dicom_stl::map<DICOMMapKey, DICOMTypeValue, group_element_compare>
{

};

#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif
