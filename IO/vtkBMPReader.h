/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.h
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
// .NAME vtkBMPReader - read Windows BMP files
// .SECTION Description
// vtkBMPReader is a source object that reads Windows BMP files.
// This includes indexed and 24bit bitmaps
//
// BMPReader creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//
// To read a volume, files must be of the form "FileName.<number>"
// (e.g., foo.ppm.0, foo.ppm.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers). 
//
// The default behavior is to read a single file. In this case, the form
// of the file is simply "FileName" (e.g., foo.bar, foo.ppm, foo.BMP). 

// .SECTION See Also
// vtkBMPWriter

#ifndef __vtkBMPReader_h
#define __vtkBMPReader_h

#include <stdio.h>
#include "vtkImageReader.h"

class VTK_IO_EXPORT vtkBMPReader : public vtkImageReader
{
public:
  static vtkBMPReader *New();
  vtkTypeRevisionMacro(vtkBMPReader,vtkImageReader);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the depth of the BMP, either 8 or 24.
  vtkGetMacro(Depth,int);
  //Description: create a clone of this object.
  virtual vtkImageReader2* MakeObject() { return vtkBMPReader::New(); }
  
  // Description: is the given file name a BMP file?
  virtual int CanReadFile(const char* fname);
  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in 
  // the format .extension
  virtual const char* GetFileExensions()
    {
      return ".bmp";
    }

  // Description: 
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
      return "Windows BMP";
    }
  
//BTX
  // Description:
  // Returns the color lut.
  vtkGetMacro(Colors,unsigned char *);
//ETX

protected:
  vtkBMPReader();
  ~vtkBMPReader();

  unsigned char *Colors;
  short Depth;
  
  virtual void ComputeDataIncrements();
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkBMPReader(const vtkBMPReader&);  // Not implemented.
  void operator=(const vtkBMPReader&);  // Not implemented.
};
#endif


