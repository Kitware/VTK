/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelimitedTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkDelimitedTextReader - reader for pulling in flat text files
//
// .SECTION Description
// vtkDelimitedTextReader is an interface for pulling in data from a
// flat, delimited text file (delimiter can be anything).
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class.

#ifndef __vtkDelimitedTextReader_h
#define __vtkDelimitedTextReader_h

#include "vtkTableAlgorithm.h"

class vtkTable;
#include <vtkStdString.h>

//BTX
struct vtkDelimitedTextReaderInternals;
//ETX

class VTK_IO_EXPORT vtkDelimitedTextReader : public vtkTableAlgorithm
{
public:
  static vtkDelimitedTextReader* New();
  vtkTypeRevisionMacro(vtkDelimitedTextReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(DelimiterString);
  vtkSetStringMacro(DelimiterString);
  vtkGetMacro(HaveHeaders,bool);
  vtkSetMacro(HaveHeaders,bool);

protected:
  vtkDelimitedTextReader();
  ~vtkDelimitedTextReader();

  vtkDelimitedTextReaderInternals* Internals;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  void OpenFile();

  char* FileName;
  char* DelimiterString;
  bool HaveHeaders;
  char *ReadBuffer;

private:
  vtkDelimitedTextReader(const vtkDelimitedTextReader&); // Not implemented
  void operator=(const vtkDelimitedTextReader&);   // Not implemented
};

#endif

