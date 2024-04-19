// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTextCodec
 * @brief   Virtual class to act as an interface for all text codecs
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
 * vtkTextCodecFactory
 *
 */

#ifndef vtkTextCodec_h
#define vtkTextCodec_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkTextCodec : public vtkObject
{
public:
  vtkTypeMacro(vtkTextCodec, vtkObject);

  ///@{
  /**
   * The name this codec goes by - should match the string the factory will take
   * to create it
   */
  virtual const char* Name();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  virtual bool CanHandle(const char* NameString);

  /**
   * is the given sample valid for this codec?  The stream will not be advanced.
   */
  virtual bool IsValid(istream& InputStream);

  ///@{
  /**
   * a base class that any output iterators need to derive from to use the first
   * signature of to_unicode.  Templates will not allow the vtable to
   * re-reference to the correct class so even though we only need the interface
   * we have to use derivation.
   */
  class OutputIterator
  {
  public:
    virtual OutputIterator& operator++(int) { return *this; }
    virtual OutputIterator& operator*() { return *this; }
    virtual OutputIterator& operator=(const vtkTypeUInt32& value) = 0;

    OutputIterator() = default;
    virtual ~OutputIterator() = default;

  private:
    OutputIterator(const OutputIterator&) = delete;
    OutputIterator& operator=(const OutputIterator&) = delete;
  };
  ///@}

  /**
   * Iterate through the sequence represented by the stream assigning the result
   * to the output iterator.  The stream will be advanced to its end so
   * subsequent use would need to reset it.
   */
  virtual void ToUnicode(istream& inputStream, vtkTextCodec::OutputIterator& output);

  /**
   * Convenience method to take data from the stream and put it into a
   * string.
   */
  std::string ToString(istream& inputStream);

  /**
   * Return the next code point from the sequence represented by the stream
   * advancing the stream through however many places needed to assemble that
   * code point.
   */
  virtual vtkTypeUInt32 NextUTF32CodePoint(istream& inputStream) = 0;

protected:
  vtkTextCodec();
  ~vtkTextCodec() override;

private:
  vtkTextCodec(const vtkTextCodec&) = delete;
  void operator=(const vtkTextCodec&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
