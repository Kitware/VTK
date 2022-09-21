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
/**
 * @class   vtkASCIITextCodec
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
 * vtkASCIITextCodecFactory
 *
 */

#ifndef vtkASCIITextCodec_h
#define vtkASCIITextCodec_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkTextCodec.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkASCIITextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkASCIITextCodec, vtkTextCodec);
  static vtkASCIITextCodec* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override;
  bool CanHandle(const char* NameString) override;
  ///@}

  /**
   * Return the next code point from the sequence represented by the begin, end iterators
   * advancing begin through however many places needed to assemble that code point
   */
  vtkTypeUInt32 NextUTF32CodePoint(istream& inputStream) override;

protected:
  vtkASCIITextCodec();
  ~vtkASCIITextCodec() override;

private:
  vtkASCIITextCodec(const vtkASCIITextCodec&) = delete;
  void operator=(const vtkASCIITextCodec&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
