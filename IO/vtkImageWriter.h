/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWriter.h
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
// .NAME vtkImageWriter - Writes images to files.
// .SECTION Description
// vtkImageWriter writes images to files with any data type. The data type of
// the file is the same scalar type as the input.  The dimensionality
// determines whether the data will be written in one or multiple files.
// This class is used as the superclass of most image writing classes 
// such as vtkBMPWriter etc. It supports streaming.

#ifndef __vtkImageWriter_h
#define __vtkImageWriter_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"

class VTK_IO_EXPORT vtkImageWriter : public vtkProcessObject
{
public:
  static vtkImageWriter *New();
  vtkTypeRevisionMacro(vtkImageWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Specify file name for the image file. You should specify either
  // a FileName or a FilePrefix. Use FilePrefix if the data is stored 
  // in multiple files.
  void SetFileName(const char *);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify file prefix for the image file(s).You should specify either
  // a FileName or FilePrefix. Use FilePrefix if the data is stored
  // in multiple files.
  void SetFilePrefix(char *filePrefix);
  vtkGetStringMacro(FilePrefix);

  // Description:
  // The sprintf format used to build filename from FilePrefix and number.
  void SetFilePattern(const char *filePattern);
  vtkGetStringMacro(FilePattern);

  // Description:
  // What dimension are the files to be written. Usually this is 2, or 3.
  // If it is 2 and the input is a volume then the volume will be 
  // written as a series of 2d slices.
  vtkSetMacro(FileDimensionality, int);
  vtkGetMacro(FileDimensionality, int);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();

protected:
  vtkImageWriter();
  ~vtkImageWriter();

  int FileDimensionality;
  char *FilePrefix;
  char *FilePattern;
  char *FileName;
  int FileNumber;
  int FileLowerLeft;
  char *InternalFileName;

  virtual void RecursiveWrite(int dim, vtkImageData *region, ofstream *file);
  virtual void RecursiveWrite(int dim, vtkImageData *cache, 
                              vtkImageData *data, ofstream *file);
  virtual void WriteFile(ofstream *file, vtkImageData *data, int extent[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *) {};
  virtual void WriteFileTrailer(ofstream *, vtkImageData *) {};
private:
  vtkImageWriter(const vtkImageWriter&);  // Not implemented.
  void operator=(const vtkImageWriter&);  // Not implemented.
};

#endif


