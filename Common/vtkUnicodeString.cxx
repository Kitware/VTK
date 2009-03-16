/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnicodeString.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUnicodeString.h"

#include <vtkObject.h>
#include <utf8.h>

#include <vtkstd/map>
#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////
// vtkUnicodeString::const_iterator

vtkUnicodeString::const_iterator::const_iterator()
{
}

vtkUnicodeString::const_iterator::const_iterator(vtkstd::string::const_iterator position) :
  Position(position)
{
}

vtkUnicodeString::value_type vtkUnicodeString::const_iterator::operator*() const
{
  return vtk_utf8::unchecked::peek_next(this->Position);
}

bool vtkUnicodeString::const_iterator::operator==(const const_iterator& rhs) const
{
  return this->Position == rhs.Position;
}

bool vtkUnicodeString::const_iterator::operator!=(const const_iterator& rhs) const
{
  return !(*this == rhs);
}

vtkUnicodeString::const_iterator& vtkUnicodeString::const_iterator::operator++()
{
  vtk_utf8::unchecked::advance(this->Position, 1);
  return *this;
}

vtkUnicodeString::const_iterator vtkUnicodeString::const_iterator::operator++(int)
{
  const_iterator result(this->Position);
  vtk_utf8::unchecked::advance(this->Position, 1);
  return result;
}

///////////////////////////////////////////////////////////////////////////
// vtkUnicodeString

vtkUnicodeString::vtkUnicodeString()
{
}

vtkUnicodeString::vtkUnicodeString(const vtkUnicodeString& rhs) :
  Storage(rhs.Storage)
{
}

vtkUnicodeString::vtkUnicodeString(size_type count, value_type character)
{
  for(size_type i = 0; i != count; ++i)
    vtk_utf8::append(character, vtkstd::back_inserter(this->Storage));
}

vtkUnicodeString::vtkUnicodeString(const_iterator first, const_iterator last) :
  Storage(first.Position, last.Position)
{
}

bool vtkUnicodeString::is_utf8(const char* value)
{
  return vtkUnicodeString::is_utf8(vtkstd::string(value ? value : ""));
}

bool vtkUnicodeString::is_utf8(const vtkstd::string& value)
{
  return vtk_utf8::is_valid(value.begin(), value.end());
}

vtkUnicodeString vtkUnicodeString::from_utf8(const char* value)
{
  return vtkUnicodeString::from_utf8(vtkstd::string(value ? value : ""));
}

vtkUnicodeString vtkUnicodeString::from_utf8(const vtkstd::string& value)
{
  vtkUnicodeString result;
  if(vtk_utf8::is_valid(value.begin(), value.end()))
    {
    result.Storage = value;
    }
  else
    {
    vtkGenericWarningMacro("vtkUnicodeString::from_utf8(): not a valid UTF-8 string.");
    }
  return result;
}

vtkUnicodeString vtkUnicodeString::from_utf16(const vtkTypeUInt16* value)
{
  vtkUnicodeString result;

  if(value)
    {
    size_type length = 0;
    while(value[length])
      ++length;

    try
      {   
      vtk_utf8::utf16to8(value, value + length, vtkstd::back_inserter(result.Storage));
      }
    catch(vtk_utf8::invalid_utf16&)
      {
      vtkGenericWarningMacro(<< "vtkUnicodeString::from_utf16(): not a valid UTF-16 string.");
      }
    }

  return result;
}

vtkUnicodeString& vtkUnicodeString::operator=(const vtkUnicodeString& rhs)
{
  if(this == &rhs)
    return *this;

  this->Storage = rhs.Storage;
  return *this;
}

vtkUnicodeString::const_iterator vtkUnicodeString::begin() const
{
  return const_iterator(this->Storage.begin());
}

vtkUnicodeString::const_iterator vtkUnicodeString::end() const
{
  return const_iterator(this->Storage.end());
}

vtkUnicodeString::value_type vtkUnicodeString::at(size_type offset) const
{
  if(offset >= this->character_count())
    throw vtkstd::out_of_range("character out-of-range");

  vtkstd::string::const_iterator iterator = this->Storage.begin();
  vtk_utf8::unchecked::advance(iterator, offset);
  return vtk_utf8::unchecked::peek_next(iterator);
}

vtkUnicodeString::value_type vtkUnicodeString::operator[](size_type offset) const
{
  vtkstd::string::const_iterator iterator = this->Storage.begin();
  vtk_utf8::unchecked::advance(iterator, offset);
  return vtk_utf8::unchecked::peek_next(iterator);
}

const char* vtkUnicodeString::utf8_str() const
{
  return this->Storage.c_str();
}

void vtkUnicodeString::utf8_str(vtkstd::string& result) const
{
  result = this->Storage;
}

vtkstd::vector<vtkTypeUInt16> vtkUnicodeString::utf16_str() const
{
  vtkstd::vector<vtkTypeUInt16> result;
  vtk_utf8::unchecked::utf8to16(this->Storage.begin(), this->Storage.end(), vtkstd::back_inserter(result));
  return result;
}

void vtkUnicodeString::utf16_str(vtkstd::vector<vtkTypeUInt16>& result) const
{
  result.clear();
  vtk_utf8::unchecked::utf8to16(this->Storage.begin(), this->Storage.end(), vtkstd::back_inserter(result));
}

vtkUnicodeString::size_type vtkUnicodeString::byte_count() const
{
  return this->Storage.size();
}

vtkUnicodeString::size_type vtkUnicodeString::character_count() const
{
  return vtk_utf8::unchecked::distance(this->Storage.begin(), this->Storage.end());
}

bool vtkUnicodeString::empty() const
{
  return this->Storage.empty();
}

vtkUnicodeString& vtkUnicodeString::operator+=(value_type value)
{
  this->push_back(value);
  return *this;
}

vtkUnicodeString& vtkUnicodeString::operator+=(const vtkUnicodeString& rhs)
{
  this->append(rhs);
  return *this;
}

void vtkUnicodeString::push_back(value_type character)
{
  try
    {
    vtk_utf8::append(character, vtkstd::back_inserter(this->Storage));
    }
  catch(vtk_utf8::invalid_code_point&)
    {
    vtkGenericWarningMacro("vtkUnicodeString::push_back(): " << character << "is not a valid Unicode code point");
    }
}

void vtkUnicodeString::append(const vtkUnicodeString& value)
{
  this->Storage.append(value.Storage);
}

void vtkUnicodeString::append(size_type count, value_type character)
{
  try
    {
    this->Storage.append(vtkUnicodeString(count, character).Storage);
    }
  catch(vtk_utf8::invalid_code_point&)
    {
    vtkGenericWarningMacro("vtkUnicodeString::append(): " << character << "is not a valid Unicode code point");
    }
}

void vtkUnicodeString::append(const_iterator first, const_iterator last)
{
#ifdef __BORLANDC__
  this->Storage.append(first.Position, last.Position - first.Position);
#else
  this->Storage.append(first.Position, last.Position);
#endif
}

void vtkUnicodeString::assign(const vtkUnicodeString& value)
{
  this->Storage.assign(value.Storage);
}

void vtkUnicodeString::assign(size_type count, value_type character)
{
  try
    {
    this->Storage.assign(vtkUnicodeString(count, character).Storage);
    }
  catch(vtk_utf8::invalid_code_point&)
    {
    vtkGenericWarningMacro("vtkUnicodeString::assign(): " << character << "is not a valid Unicode code point");
    }
}

void vtkUnicodeString::assign(const_iterator first, const_iterator last)
{
#ifdef __BORLANDC__
  this->Storage.assign(first.Position, last.Position - first.Position);
#else
  this->Storage.assign(first.Position, last.Position);
#endif
}

void vtkUnicodeString::clear()
{
  this->Storage.clear();
}

vtkUnicodeString vtkUnicodeString::fold_case() const
{
  typedef vtkstd::map<value_type, vtkUnicodeString> map_t;

  static map_t map;
  if(map.empty())
    {
    #include <vtkUnicodeCaseFoldData.h>

    for(value_type* i = &vtkUnicodeCaseFoldData[0]; *i; ++i)
      {
      const value_type code = *i;
      vtkUnicodeString mapping;
      for(++i; *i; ++i)
        {
        mapping.push_back(*i);
        }
      map.insert(vtkstd::make_pair(code, mapping));
      }
    }
  
  vtkUnicodeString result;

  for(vtkUnicodeString::const_iterator source = this->begin(); source != this->end(); ++source)
    {
    map_t::const_iterator target = map.find(*source);
    if(target != map.end())
      {
      result.append(target->second);
      }
    else
      {
      result.push_back(*source);
      }
    }

  return result;
}

int vtkUnicodeString::compare(const vtkUnicodeString& rhs) const
{
  return this->Storage.compare(rhs.Storage);
}

void vtkUnicodeString::swap(vtkUnicodeString& rhs)
{
  vtkstd::swap(this->Storage, rhs.Storage);
}

bool operator==(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) == 0;
}

bool operator!=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) != 0;
}

bool operator<(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) < 0;
}

bool operator<=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) <= 0;
}

bool operator>=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) >= 0;
}

bool operator>(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs)
{
  return lhs.compare(rhs) > 0;
}
