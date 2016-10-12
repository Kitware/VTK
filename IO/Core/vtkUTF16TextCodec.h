/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUTF16TextCodec.h

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
 * @class   vtkUTF16TextCodec
 * @brief   Class to read/write ascii text.
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
 * vtkUTF16TextCodecFactory
 *
*/

#ifndef vtkUTF16TextCodec_h
#define vtkUTF16TextCodec_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkTextCodec.h"


class VTKIOCORE_EXPORT vtkUTF16TextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkUTF16TextCodec, vtkTextCodec);
  static vtkUTF16TextCodec* New() ;
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  virtual const char* Name() ;
  virtual bool CanHandle(const char* NameString) ;
  //@}

  /**
   * Set the endianness - true if Big false is little
   */
  void SetBigEndian(bool) ;

  /**
   * Set the endianness - true if Big false is little
   */
  void FindEndianness(istream& InputStream) ;

  /**
   * is the given sample valid for this codec? - will take endianness into account
   */
  virtual bool IsValid(istream& InputStream) ;

  /**
   * Iterate through the sequence represented by the begin and end iterators assigning the result
   * to the output iterator.  This is the current pattern in vtkDelimitedTextReader
   */
  virtual void ToUnicode(istream& InputStream,
                         vtkTextCodec::OutputIterator& output) ;

  /**
   * Return the next code point from the sequence represented by the begin, end iterators
   * advancing begin through however many places needed to assemble that code point
   */
  virtual vtkUnicodeString::value_type  NextUnicode(istream& inputStream) ;

protected:
  vtkUTF16TextCodec() ;
  ~vtkUTF16TextCodec() ;

  bool _endianExplicitlySet ;
  bool _bigEndian ;

private:
  vtkUTF16TextCodec(const vtkUTF16TextCodec &) VTK_DELETE_FUNCTION;
  void operator=(const vtkUTF16TextCodec &) VTK_DELETE_FUNCTION;

};


#endif
