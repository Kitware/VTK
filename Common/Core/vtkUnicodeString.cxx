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

#include "vtkObject.h"
#include <utf8.h>

#include <map>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////
// vtkUnicodeString::const_iterator

vtkUnicodeString::const_iterator::const_iterator()
{
}

vtkUnicodeString::const_iterator::const_iterator(std::string::const_iterator position) :
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
  vtk_utf8::unchecked::next(this->Position);
  return *this;
}

vtkUnicodeString::const_iterator vtkUnicodeString::const_iterator::operator++(int)
{
  const_iterator result(this->Position);
  vtk_utf8::unchecked::next(this->Position);
  return result;
}

vtkUnicodeString::const_iterator& vtkUnicodeString::const_iterator::operator--()
{
  vtk_utf8::unchecked::prior(this->Position);
  return *this;
}

vtkUnicodeString::const_iterator vtkUnicodeString::const_iterator::operator--(int)
{
  const_iterator result(this->Position);
  vtk_utf8::unchecked::prior(this->Position);
  return result;
}

///////////////////////////////////////////////////////////////////////////
// vtkUnicodeString::back_insert_iterator

// We provide our own implementation of std::back_insert_iterator for
// use with MSVC 6, where push_back() isn't implemented for std::string.

class vtkUnicodeString::back_insert_iterator
{
public:
  back_insert_iterator(std::string& container) :
    Container(&container)
  {
  }

  back_insert_iterator& operator*()
  {
    return *this;
  }

  back_insert_iterator& operator++()
  {
    return *this;
  }

  back_insert_iterator& operator++(int)
  {
    return *this;
  }

  back_insert_iterator& operator=(std::string::const_reference value)
  {
    this->Container->push_back(value);
    return *this;
  }

private:
  std::string* Container;
};

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
    vtk_utf8::append(character, vtkUnicodeString::back_insert_iterator(this->Storage));
}

vtkUnicodeString::vtkUnicodeString(const_iterator first, const_iterator last) :
  Storage(first.Position, last.Position)
{
}

bool vtkUnicodeString::is_utf8(const char* value)
{
  return vtkUnicodeString::is_utf8(std::string(value ? value : ""));
}

bool vtkUnicodeString::is_utf8(const std::string& value)
{
  return vtk_utf8::is_valid(value.begin(), value.end());
}

vtkUnicodeString vtkUnicodeString::from_utf8(const char* value)
{
  return vtkUnicodeString::from_utf8(std::string(value ? value : ""));
}

vtkUnicodeString vtkUnicodeString::from_utf8(const char* begin, const char* end)
{
  vtkUnicodeString result;
  if(vtk_utf8::is_valid(begin, end))
    {
    result.Storage = std::string(begin, end);
    }
  else
    {
    vtkGenericWarningMacro("vtkUnicodeString::from_utf8(): not a valid UTF-8 string.");
    }
  return result;
}

vtkUnicodeString vtkUnicodeString::from_utf8(const std::string& value)
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
      vtk_utf8::utf16to8(value, value + length, vtkUnicodeString::back_insert_iterator(result.Storage));
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
    throw std::out_of_range("character out-of-range");

  std::string::const_iterator iterator = this->Storage.begin();
  vtk_utf8::unchecked::advance(iterator, offset);
  return vtk_utf8::unchecked::peek_next(iterator);
}

vtkUnicodeString::value_type vtkUnicodeString::operator[](size_type offset) const
{
  std::string::const_iterator iterator = this->Storage.begin();
  vtk_utf8::unchecked::advance(iterator, offset);
  return vtk_utf8::unchecked::peek_next(iterator);
}

const char* vtkUnicodeString::utf8_str() const
{
  return this->Storage.c_str();
}

void vtkUnicodeString::utf8_str(std::string& result) const
{
  result = this->Storage;
}

std::vector<vtkTypeUInt16> vtkUnicodeString::utf16_str() const
{
  std::vector<vtkTypeUInt16> result;
  vtk_utf8::unchecked::utf8to16(this->Storage.begin(), this->Storage.end(), std::back_inserter(result));
  return result;
}

void vtkUnicodeString::utf16_str(std::vector<vtkTypeUInt16>& result) const
{
  result.clear();
  vtk_utf8::unchecked::utf8to16(this->Storage.begin(), this->Storage.end(), std::back_inserter(result));
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

const vtkUnicodeString::size_type vtkUnicodeString::npos = std::string::npos;

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
    vtk_utf8::append(character, vtkUnicodeString::back_insert_iterator(this->Storage));
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
#if defined (__BORLANDC__) && (__BORLANDC__ < 0x0580)
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
#if defined (__BORLANDC__) && (__BORLANDC__ < 0x0580)
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
  typedef std::map<value_type, vtkUnicodeString> map_t;

  static map_t map;
  if(map.empty())
    {
    #include "vtkUnicodeCaseFoldData.h"

    for(value_type* i = &vtkUnicodeCaseFoldData[0]; *i; ++i)
      {
      const value_type code = *i;
      vtkUnicodeString mapping;
      for(++i; *i; ++i)
        {
        mapping.push_back(*i);
        }
      map.insert(std::make_pair(code, mapping));
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

vtkUnicodeString vtkUnicodeString::substr(size_type offset, size_type count) const
{
  std::string::const_iterator from = this->Storage.begin();
  std::string::const_iterator last = this->Storage.end();

  while(from != last && offset--)
    vtk_utf8::unchecked::advance(from, 1);

  std::string::const_iterator to = from;
  while(to != last && count--)
    vtk_utf8::unchecked::advance(to, 1);

  return vtkUnicodeString(from, to);
}

void vtkUnicodeString::swap(vtkUnicodeString& rhs)
{
  std::swap(this->Storage, rhs.Storage);
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
