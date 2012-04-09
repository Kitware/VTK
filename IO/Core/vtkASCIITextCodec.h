/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkASCIITextCodec.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkASCIITextCodec - Class to read/write ascii text.
//
// .SECTION Description
// A virtual class interface for codecs that readers/writers can rely on
//
// .SECTION Thanks
// Thanks to Tim Shed from Sandia National Laboratories for his work
// on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
// keeping me out of the weeds
//
// .SECTION See Also
// vtkASCIITextCodecFactory
//

#ifndef __vtkASCIITextCodec_h
#define __vtkASCIITextCodec_h

#include "vtkTextCodec.h"


class VTK_IO_EXPORT vtkASCIITextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkASCIITextCodec, vtkTextCodec);
  static vtkASCIITextCodec* New() ;
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name this codec goes by - should match the string the factory will take to create it
  virtual const char* Name();
  virtual bool CanHandle(const char* NameString);

  // Description:
  // is the given sample valid for this codec?
  virtual bool IsValid(istream& InputStream);


  // Description:
  // Iterate through the sequence represented by the stream assigning the result
  // to the output iterator.  The stream will be advanced to its end so subsequent use
  // would need to reset it.
  virtual void ToUnicode(istream& InputStream,
                         vtkTextCodec::OutputIterator& output) ;

  // Description:
  // Return the next code point from the sequence represented by the stream
  // advancing the stream through however many places needed to assemble that code point
  virtual vtkUnicodeString::value_type NextUnicode(istream& inputStream) ;

//BTX
protected:
  vtkASCIITextCodec() ;
  ~vtkASCIITextCodec() ;

private:
  vtkASCIITextCodec(const vtkASCIITextCodec &) ; // Not implemented.
  void operator=(const vtkASCIITextCodec &) ; // Not implemented.

//ETX
};


#endif
