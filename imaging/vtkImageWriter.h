/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

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
// .NAME vtkImageWriter - Writes no header files.
// .SECTION Description
// vtkImageWriter writes no header files with any data type. The data type
// of the file is the same scalar type as the input.
// The dimensionality determines whether the data will be written in one or
// multiple files.


#ifndef __vtkImageWriter_h
#define __vtkImageWriter_h

#include <iostream.h>
#include <fstream.h>
#include "vtkObject.h"
#include "vtkImageCache.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkImageRegion.h"

class VTK_EXPORT vtkImageWriter : public vtkObject
{
public:
  vtkImageWriter();
  ~vtkImageWriter();
  static vtkImageWriter *New() {return new vtkImageWriter;};
  const char *GetClassName() {return "vtkImageWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetFilePrefix(char *filePrefix);
  void SetFilePattern(char *filePattern);

  vtkSetMacro(FileDimensionality, int);
  vtkGetMacro(FileDimensionality, int);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  vtkSetObjectMacro(Input,vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();

  // Public for templated function
  char *FileName;

protected:
  vtkImageCache *Input;
  int FileDimensionality;
  char *FilePrefix;
  char *FilePattern;
  int FileNumber;

  void RecursiveWrite(int dim, vtkImageRegion *region);
  virtual void WriteFile(vtkImageRegion *region);
  virtual void WriteFileHeader(ofstream *file, vtkImageRegion *region)
    {file = file; region = region;}
};

#endif


