/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPNGWriter
 * @brief   Writes PNG files.
 *
 * vtkPNGWriter writes PNG files. It supports 1 to 4 component data of
 * unsigned char or unsigned short
 *
 * @sa
 * vtkPNGReader
*/

#ifndef vtkPNGWriter_h
#define vtkPNGWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkImageData;
class vtkUnsignedCharArray;

class VTKIOIMAGE_EXPORT vtkPNGWriter : public vtkImageWriter
{
public:
  static vtkPNGWriter *New();
  vtkTypeMacro(vtkPNGWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The main interface which triggers the writer to start.
   */
  void Write() override;

  //@{
  /**
   * Set/Get the zlib compression level.
   * The range is 0-9, with 0 meaning no compression
   * corresponding to the largest file size, and 9 meaning
   * best compression, corresponding to the smallest file size.
   * The default is 5.
   */
  vtkSetClampMacro(CompressionLevel, int, 0, 9);
  vtkGetMacro(CompressionLevel, int);
  //@}

  //@{
  /**
   * Write the image to memory (a vtkUnsignedCharArray)
   */
  vtkSetMacro(WriteToMemory, vtkTypeUBool);
  vtkGetMacro(WriteToMemory, vtkTypeUBool);
  vtkBooleanMacro(WriteToMemory, vtkTypeUBool);
  //@}

  //@{
  /**
   * When writing to memory this is the result, it will be nullptr until the
   * data is written the first time
   */
  virtual void SetResult(vtkUnsignedCharArray*);
  vtkGetObjectMacro(Result, vtkUnsignedCharArray);
  //@}

  /**
   * Adds a text chunk to the PNG. More than one text chunk with the same key is permissible.
   * There are a number of predefined keywords that should be used
   * when appropriate. See
   * http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
   * for more information.
   */
  void AddText(const char* key, const char* value);
  //@{
  /**
   * Standard keys
   */
  static const char* TITLE;
  static const char* AUTHOR;
  static const char* DESCRIPTION;
  static const char* COPYRIGHT;
  static const char* CREATION_TIME;
  static const char* SOFTWARE;
  static const char* DISCLAIMER;
  static const char* WARNING;
  static const char* SOURCE;
  static const char* COMMENT;
  //@}

protected:
  vtkPNGWriter();
  ~vtkPNGWriter() override;

  void WriteSlice(vtkImageData *data, int* uExtent);
  int CompressionLevel;
  vtkUnsignedCharArray *Result;
  FILE *TempFP;
  class vtkInternals;
  vtkInternals* Internals;


private:
  vtkPNGWriter(const vtkPNGWriter&) = delete;
  void operator=(const vtkPNGWriter&) = delete;
};

#endif
