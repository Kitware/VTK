/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64Utility.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBase64Utility - base64 encode and decode utilities.
// .SECTION Description
// vtkBase64Utility implements base64 encoding and decoding.

#ifndef __vtkBase64Utility_h
#define __vtkBase64Utility_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkBase64Utility : public vtkObject
{
public:
  static vtkBase64Utility *New();
  vtkTypeRevisionMacro(vtkBase64Utility,vtkObject);

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
  static unsigned long Encode(const unsigned char *input, 
                              unsigned long length, 
                              unsigned char *output);


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
  static unsigned long Decode(const unsigned char *input, 
                              unsigned long length, 
                              unsigned char *output);

protected:
  vtkBase64Utility() {};
  ~vtkBase64Utility() {};  
  
private:
  vtkBase64Utility(const vtkBase64Utility&);  // Not implemented.
  void operator=(const vtkBase64Utility&);  // Not implemented.
};

#endif
