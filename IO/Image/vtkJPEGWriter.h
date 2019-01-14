/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkJPEGWriter
 * @brief   Writes JPEG files.
 *
 * vtkJPEGWriter writes JPEG files. It supports 1 and 3 component data of
 * unsigned char. It relies on the IJG's libjpeg.  Thanks to IJG for
 * supplying a public jpeg IO library.
 *
 * @sa
 * vtkJPEGReader
*/

#ifndef vtkJPEGWriter_h
#define vtkJPEGWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkUnsignedCharArray;
class vtkImageData;

class VTKIOIMAGE_EXPORT vtkJPEGWriter : public vtkImageWriter
{
public:
  static vtkJPEGWriter *New();
  vtkTypeMacro(vtkJPEGWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The main interface which triggers the writer to start.
   */
  void Write() override;

  //@{
  /**
   * Compression quality. 0 = Low quality, 100 = High quality
   */
  vtkSetClampMacro(Quality, int, 0, 100);
  vtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Progressive JPEG generation.
   */
  vtkSetMacro(Progressive, vtkTypeUBool);
  vtkGetMacro(Progressive, vtkTypeUBool);
  vtkBooleanMacro(Progressive, vtkTypeUBool);
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

protected:
  vtkJPEGWriter();
  ~vtkJPEGWriter() override;

  void WriteSlice(vtkImageData *data, int* uExtent);

private:
  int Quality;
  vtkTypeUBool Progressive;
  vtkUnsignedCharArray *Result;
  FILE *TempFP;

private:
  vtkJPEGWriter(const vtkJPEGWriter&) = delete;
  void operator=(const vtkJPEGWriter&) = delete;
};

#endif


