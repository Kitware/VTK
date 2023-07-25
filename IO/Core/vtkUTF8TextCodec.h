// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkUTF8TextCodec : public vtkTextCodec
{
public:
  vtkTypeMacro(vtkUTF8TextCodec, vtkTextCodec);
  static vtkUTF8TextCodec* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override { return "UTF-8"; }

  /**
   * Iterate through the sequence represented by the stream assigning the result
   * to the output iterator.  The stream will be advanced to its end so subsequent use
   * would need to reset it.
   */
  void ToUnicode(istream& InputStream, vtkTextCodec::OutputIterator& output) override;

  /**
   * Return the next code point from the sequence represented by the begin, end iterators
   * advancing begin through however many places needed to assemble that code point
   */
  vtkTypeUInt32 NextUTF32CodePoint(istream& inputStream) override;

protected:
  vtkUTF8TextCodec();
  ~vtkUTF8TextCodec() override;

private:
  vtkUTF8TextCodec(const vtkUTF8TextCodec&) = delete;
  void operator=(const vtkUTF8TextCodec&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
