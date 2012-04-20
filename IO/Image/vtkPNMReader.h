/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPNMReader - read pnm (i.e., portable anymap) files

// .SECTION Description
// vtkPNMReader is a source object that reads pnm (portable anymap) files.
// This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
// (Currently this object only reads binary versions of these files.)
//
// PNMReader creates structured point datasets. The dimension of the
// dataset depends upon the number of files read. Reading a single file
// results in a 2D image, while reading more than one file results in a
// 3D volume.
//
// To read a volume, files must be of the form "FileName.<number>" (e.g.,
// foo.ppm.0, foo.ppm.1, ...). You must also specify the DataExtent.  The
// fifth and sixth values of the DataExtent specify the beginning and ending
// files to read.


#ifndef __vtkPNMReader_h
#define __vtkPNMReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"

class VTKIOIMAGE_EXPORT vtkPNMReader : public vtkImageReader
{
public:
  static vtkPNMReader *New();
  vtkTypeMacro(vtkPNMReader,vtkImageReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  int CanReadFile(const char* fname);
  // Description:
  // .pnm .pgm .ppm
  virtual const char* GetFileExtensions()
    {
      return ".pnm .pgm .ppm";
    }

  // Description:
  // PNM
  virtual const char* GetDescriptiveName()
    {
      return "PNM";
    }

protected:
  vtkPNMReader() {};
  ~vtkPNMReader() {};
  void ExecuteInformation();
private:
  vtkPNMReader(const vtkPNMReader&);  // Not implemented.
  void operator=(const vtkPNMReader&);  // Not implemented.
};

#endif


