// SPDX-FileCopyrightText: Copyright (c) 2003 Matt Turek
// SPDX-License-Identifier: BSD-4-Clause
#ifndef __DICOM_PARSER_MAP__H_
#define __DICOM_PARSER_MAP__H_

#ifdef _MSC_VER
#pragma warning(disable : 4514)
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#pragma warning(disable : 4710)
#pragma warning(disable : 4702)
#pragma warning(push, 3)
#endif

#include <map>
#include <utility>

#include "DICOMConfig.h"

VTK_ABI_NAMESPACE_BEGIN
class DICOMCallback;

//
// Structure that implements a compare operator for
// a pair of doublebytes.  This is used when comparing
// group, element pairs.
//
struct group_element_compare
{
  bool operator()(
    const std::pair<doublebyte, doublebyte>& p1, const std::pair<doublebyte, doublebyte>& p2) const
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
typedef std::pair<doublebyte, doublebyte> DICOMMapKeyOverride;

// DICOM_EXPIMP_TEMPLATE template struct DICOM_MAP_EXPORT std::pair<doublebyte,
// doublebyte>;
//
// Subclass of a pair of doublebytes to make
// type names shorter in the code.
//
class DICOMMapKey : public DICOMMapKeyOverride
{
public:
  DICOMMapKey(doublebyte v1, doublebyte v2)
    : std::pair<doublebyte, doublebyte>(v1, v2)
  {
  }
};

//
// Typedef of a pair of doublebyte, vector.
//
typedef std::pair<doublebyte, std::vector<DICOMCallback*>*> DICOMMapValueOverride;

// DICOM_EXPIMP_TEMPLATE template struct DICOM_MAP_EXPORT std::pair<doublebyte,
// std::vector<DICOMCallback*>*>;

//
// Subclass of pair doublebyte, vector<DICOMCallback*>.
// Makes types shorter in the code.
//
class DICOMMapValue : public DICOMMapValueOverride
{
public:
  DICOMMapValue() = default;

  DICOMMapValue(doublebyte v1, std::vector<DICOMCallback*>* v2)
    : std::pair<doublebyte, std::vector<DICOMCallback*>*>(v1, v2)
  {
  }
};

// DICOM_EXPIMP_TEMPLATE template class DICOM_MAP_EXPORT std::map<DICOMMapKey,
// DICOMMapValue, group_element_compare>;

//
// Subclass of the particular map we're using.  Again,
// makes type specifiers shorter in the code.
//
class DICOMParserMap : public std::map<DICOMMapKey, DICOMMapValue, group_element_compare>
{
};

typedef doublebyte DICOMTypeValue;

// DICOM_EXPIMP_TEMPLATE template class  std::map<DICOMMapKey, DICOMTypeValue,
// group_element_compare>;

class DICOMImplicitTypeMap : public std::map<DICOMMapKey, DICOMTypeValue, group_element_compare>
{
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

VTK_ABI_NAMESPACE_END
#endif
