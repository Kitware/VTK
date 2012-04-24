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
// .NAME vtkBase64Utilities - base64 encode and decode utilities.
// .SECTION Description
// vtkBase64Utilities implements base64 encoding and decoding.

#ifndef __vtkBase64Utilities_h
#define __vtkBase64Utilities_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKIOCORE_EXPORT vtkBase64Utilities : public vtkObject
{
public:
  static vtkBase64Utilities *New();
  vtkTypeMacro(vtkBase64Utilities,vtkObject);

  // Description:
  // Encode 3 bytes into 4 bytes
  static void EncodeTriplet(unsigned char i0,
                            unsigned char i1,
                            unsigned char i2,
                            unsigned char *o0,
                            unsigned char *o1,
                            unsigned char *o2,
                            unsigned char *o3);

  // Description:
  // Encode 2 bytes into 4 bytes
  static void EncodePair(unsigned char i0,
                         unsigned char i1,
                         unsigned char *o0,
                         unsigned char *o1,
                         unsigned char *o2,
                         unsigned char *o3);

  // Description:
  // Encode 1 byte into 4 bytes
  static void EncodeSingle(unsigned char i0,
                           unsigned char *o0,
                           unsigned char *o1,
                           unsigned char *o2,
                           unsigned char *o3);

  // Description:
  // Encode 'length' bytes from the input buffer and store the
  // encoded stream into the output buffer. Return the length of
  // the encoded stream. Note that the output buffer must be allocated
  // by the caller (length * 1.5 should be a safe estimate).
  // If 'mark_end' is true than an extra set of 4 bytes is added
  // to the end of the stream if the input is a multiple of 3 bytes.
  // These bytes are invalid chars and therefore they will stop the decoder
  // thus enabling the caller to decode a stream without actually knowing
  // how much data to expect (if the input is not a multiple of 3 bytes then
  // the extra padding needed to complete the encode 4 bytes will stop the
  // decoding anyway).
  static unsigned long Encode(const unsigned char *input,
                              unsigned long length,
                              unsigned char *output,
                              int mark_end = 0);


  // Description:
  // Decode 4 bytes into 3 bytes.
  static int DecodeTriplet(unsigned char i0,
                           unsigned char i1,
                           unsigned char i2,
                           unsigned char i3,
                           unsigned char *o0,
                           unsigned char *o1,
                           unsigned char *o2);

  // Description:
  // Decode bytes from the input buffer and store the decoded stream
  // into the output buffer until 'length' bytes have been decoded.
  // Return the real length of the decoded stream (which should be equal to
  // 'length'). Note that the output buffer must be allocated by the caller.
  // If 'max_input_length' is not null, then it specifies the number of
  // encoded bytes that should be at most read from the input buffer. In
  // that case the 'length' parameter is ignored. This enables the caller
  // to decode a stream without actually knowing how much decoded data to
  // expect (of course, the buffer must be large enough).
  static unsigned long Decode(const unsigned char *input,
                              unsigned long length,
                              unsigned char *output,
                              unsigned long max_input_length = 0);

protected:
  vtkBase64Utilities() {};
  ~vtkBase64Utilities() {};

private:
  vtkBase64Utilities(const vtkBase64Utilities&);  // Not implemented.
  void operator=(const vtkBase64Utilities&);  // Not implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkBase64Utilities.h
