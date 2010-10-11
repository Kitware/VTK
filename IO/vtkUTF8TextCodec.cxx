/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUTF8TextCodec.cxx

Copyright (c)
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkUTF8TextCodec.h>

#include <vtkObjectFactory.h>
#include <vtkTextCodecFactory.h>

#include <utf8.h>


vtkStandardNewMacro(vtkUTF8TextCodec);

vtkTextCodec* vtkUTF8TextCodecFromCallback()
{
   return vtkUTF8TextCodec::New() ;
}

class vtkUTF8TextCodecRegister
{
  public:
    vtkUTF8TextCodecRegister()
    {
      vtkTextCodecFactory::RegisterCreateCallback(vtkUTF8TextCodecFromCallback) ;
    }
} ;


static vtkUTF8TextCodecRegister foo ;


const bool vtkUTF8TextCodec::CanHandle(const char* testStr)
{
  if (0 == strcmp(testStr, "UTF-8"))
    return true ;
  else
    return false ;
}


namespace
{
  // iterator to use in testing validity - throws all input away.
  class testIterator : public vtkTextCodec::OutputIterator
  {
  public:
    virtual testIterator& operator++(int) {return *this ;}
    virtual testIterator& operator*() {return *this ;}
    virtual testIterator& operator=(const vtkUnicodeString::value_type value) {return *this ;}

    testIterator() {}
    ~testIterator() {}

  private:
    testIterator(const testIterator&) ; // Not implemented
    const testIterator& operator=(const testIterator&) ; // Not Implemented
  } ;


} // end anonymous namespace


bool vtkUTF8TextCodec::IsValid(istream& InputStream)
{
  bool returnBool = true ;
  // get the position of the stream so we can restore it when we are done
  istream::pos_type StreamPos = InputStream.tellg() ;

  try
    {
    testIterator junk ;
    this->ToUnicode(InputStream, junk) ;
    }
  catch(...)
    {
    returnBool = false ;
    }

  // reset the stream
  InputStream.clear() ;
  InputStream.seekg(StreamPos) ;

  return returnBool ;
}


void vtkUTF8TextCodec::ToUnicode(istream& InputStream,
                                 vtkTextCodec::OutputIterator& Output)
{
  while (!InputStream.eof())
    {
    try
      {
      if (1 <= InputStream.rdbuf()->in_avail())
        {
        vtkUnicodeString::value_type CodePoint = this->NextUnicode(InputStream) ;
        *Output++ = CodePoint ;
        }
      else
        break ;
      }
    catch (std::string& ef)
      {
      if (ef == "End of Input")
        return ; // we just completed the sequence...
      else
        throw ef ;
      }
    }
}


vtkUnicodeString::value_type vtkUTF8TextCodec::NextUnicode(istream& InputStream)
{
  istream::char_type c[5] ;
  c[4] = '\0' ;

  unsigned int getSize = InputStream.readsome(c, 5) ;
  if (0 == getSize)
    throw(std::string("End of Input")) ;

  istream::char_type* c1 = c ;

  const vtkTypeUInt32 code_point = vtk_utf8::next(c1, &c[getSize]) ;

  unsigned int nCharExtracted = c1 - c ;
  unsigned int nCharReturned = getSize - nCharExtracted ;
  for (int j = 0; j < nCharReturned; ++j)
    InputStream.unget() ;

  return code_point ;
}


vtkUTF8TextCodec::vtkUTF8TextCodec() : vtkTextCodec()
{
}


vtkUTF8TextCodec::~vtkUTF8TextCodec()
{
}
