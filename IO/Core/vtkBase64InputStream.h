/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64InputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBase64InputStream
 * @brief   Reads base64-encoded input from a stream.
 *
 * vtkBase64InputStream implements base64 decoding with the
 * vtkInputStream interface.
*/

#ifndef vtkBase64InputStream_h
#define vtkBase64InputStream_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkInputStream.h"

class VTKIOCORE_EXPORT vtkBase64InputStream : public vtkInputStream
{
public:
  vtkTypeMacro(vtkBase64InputStream,vtkInputStream);
  static vtkBase64InputStream *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called after the stream position has been set by the caller, but
   * before any Seek or Read calls.  The stream position should not be
   * adjusted by the caller until after an EndReading call.
   */
  void StartReading() override;

  /**
   * Seek to the given offset in the input data.  Returns 1 for
   * success, 0 for failure.
   */
  int Seek(vtkTypeInt64 offset) override;

  /**
   * Read input data of the given length.  Returns amount actually
   * read.
   */
  size_t Read(void* data, size_t length) override;

  /**
   * Called after all desired calls to Seek and Read have been made.
   * After this call, the caller is free to change the position of the
   * stream.  Additional reads should not be done until after another
   * call to StartReading.
   */
  void EndReading() override;

protected:
  vtkBase64InputStream();
  ~vtkBase64InputStream() override;

  // Number of decoded bytes left in Buffer from last call to Read.
  int BufferLength;
  unsigned char Buffer[2];

  // Reads 4 bytes from the input stream and decodes them into 3 bytes.
  int DecodeTriplet(unsigned char& c0, unsigned char& c1, unsigned char& c2);

private:
  vtkBase64InputStream(const vtkBase64InputStream&) = delete;
  void operator=(const vtkBase64InputStream&) = delete;
};

#endif
