/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutputStream
 * @brief   Wraps a binary output stream with a VTK interface.
 *
 * vtkOutputStream provides a VTK-style interface wrapping around a
 * standard output stream.  The access methods are virtual so that
 * subclasses can transparently provide encoding of the output.  Data
 * lengths for Write calls refer to the length of the data in memory.
 * The actual length in the stream may differ for subclasses that
 * implement an encoding scheme.
*/

#ifndef vtkOutputStream_h
#define vtkOutputStream_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKIOCORE_EXPORT vtkOutputStream : public vtkObject
{
public:
  vtkTypeMacro(vtkOutputStream,vtkObject);
  static vtkOutputStream *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the real output stream.
   */
  vtkSetMacro(Stream, ostream*);
  vtkGetMacro(Stream, ostream*);
  //@}

  /**
   * Called after the stream position has been set by the caller, but
   * before any Write calls.  The stream position should not be
   * adjusted by the caller until after an EndWriting call.
   */
  virtual int StartWriting();

  /**
   * Write output data of the given length.
   */
  virtual int Write(void const* data, size_t length);

  /**
   * Called after all desired calls to Write have been made.  After
   * this call, the caller is free to change the position of the
   * stream.  Additional writes should not be done until after another
   * call to StartWriting.
   */
  virtual int EndWriting();

protected:
  vtkOutputStream();
  ~vtkOutputStream() override;

  // The real output stream.
  ostream* Stream;
  int WriteStream(const char* data, size_t length);

private:
  vtkOutputStream(const vtkOutputStream&) = delete;
  void operator=(const vtkOutputStream&) = delete;
};

#endif
