/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64OutputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBase64OutputStream
 * @brief   Writes base64-encoded output to a stream.
 *
 * vtkBase64OutputStream implements base64 encoding with the
 * vtkOutputStream interface.
*/

#ifndef vtkBase64OutputStream_h
#define vtkBase64OutputStream_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkOutputStream.h"

class VTKIOCORE_EXPORT vtkBase64OutputStream : public vtkOutputStream
{
public:
  vtkTypeMacro(vtkBase64OutputStream,vtkOutputStream);
  static vtkBase64OutputStream *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called after the stream position has been set by the caller, but
   * before any Write calls.  The stream position should not be
   * adjusted by the caller until after an EndWriting call.
   */
  int StartWriting() override;

  /**
   * Write output data of the given length.
   */
  int Write(void const* data, size_t length) override;

  /**
   * Called after all desired calls to Write have been made.  After
   * this call, the caller is free to change the position of the
   * stream.  Additional writes should not be done until after another
   * call to StartWriting.
   */
  int EndWriting() override;

protected:
  vtkBase64OutputStream();
  ~vtkBase64OutputStream() override;

  // Number of un-encoded bytes left in Buffer from last call to Write.
  unsigned int BufferLength;
  unsigned char Buffer[2];

  // Methods to encode and write data.
  int EncodeTriplet(unsigned char c0, unsigned char c1, unsigned char c2);
  int EncodeEnding(unsigned char c0, unsigned char c1);
  int EncodeEnding(unsigned char c0);

private:
  vtkBase64OutputStream(const vtkBase64OutputStream&) = delete;
  void operator=(const vtkBase64OutputStream&) = delete;
};

#endif
