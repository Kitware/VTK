/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUTF8TextCodec.h

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
/**
 * @class   vtkUTF8TextCodec
 * @brief   Class to read/write UTF-8 text
 *
 *
 * A virtual class interface for codecs that readers/writers can rely on
 *
 * @par Thanks:
 * Thanks to Tim Shed from Sandia National Laboratories for his work
 * on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
 * keeping me out of the weeds
 *
 * @sa
 * vtkUTF8TextCodecFactory
 *
*/

#ifndef vtkUTF8TextCodec_h
#define vtkUTF8TextCodec_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkTextCodec.h"


class VTKIOCORE_EXPORT vtkUTF8TextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkUTF8TextCodec, vtkTextCodec);
  static vtkUTF8TextCodec* New() ;
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  virtual const char* Name() {return "UTF-8" ;}
  virtual bool CanHandle(const char* testStr);

  /**
   * is the given sample valid for this codec?
   */
  virtual bool IsValid(istream& InputStream) ;

  /**
   * Iterate through the sequence represented by the stream assigning the result
   * to the output iterator.  The stream will be advanced to its end so subsequent use
   * would need to reset it.
   */
  virtual void ToUnicode(istream& InputStream,
                         vtkTextCodec::OutputIterator& output) ;

  /**
   * Return the next code point from the sequence represented by the stream
   * advancing the stream through however many places needed to assemble that code point
   */
  virtual vtkUnicodeString::value_type NextUnicode(istream& inputStream) ;

protected:
  vtkUTF8TextCodec() ;
  ~vtkUTF8TextCodec() ;

private:
  vtkUTF8TextCodec(const vtkUTF8TextCodec &) VTK_DELETE_FUNCTION;
  void operator=(const vtkUTF8TextCodec &) VTK_DELETE_FUNCTION;

};


#endif
