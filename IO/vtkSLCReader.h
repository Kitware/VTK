/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLCReader.h
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

// .NAME vtkSLCReader - read an SLC volume file.
// .SECTION Description
// vtkSLCReader reads an SLC file and creates a structured point dataset.
// The size of the volume and the data spacing is set from the SLC file
// header.

#ifndef __vtkSLCReader_h
#define __vtkSLCReader_h

#include <stdio.h>
#include "vtkImageReader2.h"

class VTK_IO_EXPORT vtkSLCReader : public vtkImageReader2 
{
public:
  static vtkSLCReader *New();
  vtkTypeRevisionMacro(vtkSLCReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the name of the file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Was there an error on the last read performed?
  vtkGetMacro(Error,int);
  //Description: create a clone of this object.
  virtual vtkImageReader2* MakeObject() { return vtkSLCReader::New(); }
  
  // Description:
  // Is the given file an SLC file?
  int CanReadFile(const char* fname);
  // Description:
  // .slc
  virtual const char* GetFileExensions()
    {
      return ".slc";
    }

  // Description: 
  // SLC 
  virtual const char* GetDescriptiveName()
    {
      return "SLC";
    }
  
protected:
  vtkSLCReader();
  ~vtkSLCReader();

  // Reads the file name and builds a vtkStructuredPoints dataset.
  virtual void ExecuteData(vtkDataObject*);

  // Not used now, but will be needed when this is made into an 
  // imaging filter.
  // Sets WholeExtent, Origin, Spacing, ScalarType 
  // and NumberOfComponents of output.
  void ExecuteInformation();
  
  // Decodes an array of eight bit run-length encoded data.
  unsigned char *Decode8BitData( unsigned char *in_ptr, int size );
  int Error;
private:
  vtkSLCReader(const vtkSLCReader&);  // Not implemented.
  void operator=(const vtkSLCReader&);  // Not implemented.
};

#endif


