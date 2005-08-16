/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkMPEG2Writer - Writes Windows Movie files.
// .SECTION Description
// vtkMPEG2Writer writes Movie files. The data type
// of the file is unsigned char regardless of the input type.

// .SECTION See Also
// vtkMovieReader

#ifndef __vtkMPEG2WriterHelper_h
#define __vtkMPEG2WriterHelper_h

#include "vtkProcessObject.h"

class vtkMPEG2WriterInternal;
class vtkImageData;
struct vtkMPEG2Structure;

class VTK_EXPORT vtkMPEG2WriterHelper : public vtkProcessObject
{
public:
  static vtkMPEG2WriterHelper *New();
  vtkTypeRevisionMacro(vtkMPEG2WriterHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Specify file name of avi file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the input object from the image pipeline.
  virtual void SetInput(vtkImageData *input);
  virtual vtkImageData *GetInput();

  // Description:
  // Was there an error on the last read performed?
  vtkGetMacro(Error,int);

  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  void Start();
  void Write();
  void End();
  
protected:
  vtkMPEG2WriterHelper();
  ~vtkMPEG2WriterHelper();

  char *FileName;
  vtkMPEG2WriterInternal *Internals;
  int Error;

  long Time;
  int ActualWrittenTime;

  void Initialize();

  int Initialized;

  vtkMPEG2Structure* MPEGStructure;

private:
  vtkMPEG2WriterHelper(const vtkMPEG2WriterHelper&); // Not implemented
  void operator=(const vtkMPEG2WriterHelper&); // Not implemented
};

#endif



