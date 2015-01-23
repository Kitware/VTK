/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// Usually, all BMPs are converted to 24bit RGB, but BMPs may be output
// as 8bit images with a LookupTable if the Allow8BitBMP flag is set.
//
// BMPReader creates structured point datasets. The dimension of the
// dataset depends upon the number of files read. Reading a single file
// results in a 2D image, while reading more than one file results in a
// 3D volume.
//
// To read a volume, files must be of the form "FileName.<number>"
// (e.g., foo.bmp.0, foo.bmp.1, ...). You must also specify the image
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers).
//
// The default behavior is to read a single file. In this case, the form
// of the file is simply "FileName" (e.g., foo.bmp).

// .SECTION See Also
// vtkBMPWriter

#ifndef vtkBMPReader_h
#define vtkBMPReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"
class vtkLookupTable;

class VTKIOIMAGE_EXPORT vtkBMPReader : public vtkImageReader
{
public:
  static vtkBMPReader *New();
  vtkTypeMacro(vtkBMPReader,vtkImageReader);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the depth of the BMP, either 8 or 24.
  vtkGetMacro(Depth,int);

  // Description:
  // Is the given file a BMP file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
      return ".bmp";
    }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
      return "Windows BMP";
    }

  // Description:
  // If this flag is set and the BMP reader encounters an 8bit file,
  // the data will be kept as unsigned chars and a lookuptable will be
  // exported
  vtkSetMacro(Allow8BitBMP,int);
  vtkGetMacro(Allow8BitBMP,int);
  vtkBooleanMacro(Allow8BitBMP,int);

  vtkGetObjectMacro(LookupTable, vtkLookupTable);

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
  int Allow8BitBMP;
  vtkLookupTable *LookupTable;

  virtual void ComputeDataIncrements();
  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation* outInfo);
private:
  vtkBMPReader(const vtkBMPReader&);  // Not implemented.
  void operator=(const vtkBMPReader&);  // Not implemented.
};
#endif


