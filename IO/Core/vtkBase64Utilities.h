/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64Utilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBase64Utilities
 * @brief   base64 encode and decode utilities.
 *
 * vtkBase64Utilities implements base64 encoding and decoding.
*/

#ifndef vtkBase64Utilities_h
#define vtkBase64Utilities_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKIOCORE_EXPORT vtkBase64Utilities : public vtkObject
{
public:
  static vtkBase64Utilities *New();
  vtkTypeMacro(vtkBase64Utilities,vtkObject);

  /**
   * Encode 3 bytes into 4 bytes
   */
  static void EncodeTriplet(unsigned char i0,
                            unsigned char i1,
                            unsigned char i2,
                            unsigned char *o0,
                            unsigned char *o1,
                            unsigned char *o2,
                            unsigned char *o3);

  /**
   * Encode 2 bytes into 4 bytes
   */
  static void EncodePair(unsigned char i0,
                         unsigned char i1,
                         unsigned char *o0,
                         unsigned char *o1,
                         unsigned char *o2,
                         unsigned char *o3);

  /**
   * Encode 1 byte into 4 bytes
   */
  static void EncodeSingle(unsigned char i0,
                           unsigned char *o0,
                           unsigned char *o1,
                           unsigned char *o2,
                           unsigned char *o3);

  /**
   * Encode 'length' bytes from the input buffer and store the
   * encoded stream into the output buffer. Return the length of
   * the encoded stream. Note that the output buffer must be allocated
   * by the caller (length * 1.5 should be a safe estimate).
   * If 'mark_end' is true then an extra set of 4 bytes is added
   * to the end of the stream if the input is a multiple of 3 bytes.
   * These bytes are invalid chars and therefore they will stop the decoder
   * thus enabling the caller to decode a stream without actually knowing
   * how much data to expect (if the input is not a multiple of 3 bytes then
   * the extra padding needed to complete the encode 4 bytes will stop the
   * decoding anyway).
   */
  static unsigned long Encode(const unsigned char *input,
                              unsigned long length,
                              unsigned char *output,
                              int mark_end = 0);

  /**
   * Decode 4 bytes into 3 bytes.
   * Return the number of bytes actually decoded (0 to 3, inclusive).
   */
  static int DecodeTriplet(unsigned char i0,
                           unsigned char i1,
                           unsigned char i2,
                           unsigned char i3,
                           unsigned char *o0,
                           unsigned char *o1,
                           unsigned char *o2);

  /**
   * Decode 4 bytes at a time from the input buffer and store the decoded
   * stream into the output buffer. The required output buffer size must be
   * determined and allocated by the caller. The needed output space is
   * always less than the input buffer size, so a good first order
   * approximation is to allocate the same size. Base64 encoding is about
   * 4/3 overhead, so a tighter bound is possible.
   * Return the number of bytes atually placed into the output buffer.
   */
  static size_t DecodeSafely(const unsigned char *input,
                             size_t inputLen,
                             unsigned char *output,
                             size_t outputLen);

protected:
  vtkBase64Utilities() {}
  ~vtkBase64Utilities() override {}

private:
  vtkBase64Utilities(const vtkBase64Utilities&) = delete;
  void operator=(const vtkBase64Utilities&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkBase64Utilities.h
