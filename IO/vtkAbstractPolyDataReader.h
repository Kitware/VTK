/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractPolyDataReader - Superclass of polydata file readers.
// .SECTION Description
// vtkAbstractPolyDataReader is the parent class for several
// vtkPolyData readers.  vtkPolyDataReader was implemented
// before vtkAbstractPolyDataReader, vtkAbstractPolyDataReader
// is intended to have a simpler interface.

// .SECTION See Also
// vtkSTLReader vtkBYUReader vtkLegacyPolyDataReader

#ifndef __vtkAbstractPolyDataReader_h
#define __vtkAbstractPolyDataReader_h

#include "vtkPolyDataAlgorithm.h"

class vtkStringArray;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_IO_EXPORT vtkAbstractPolyDataReader : public vtkPolyDataAlgorithm
{
public:
  static vtkAbstractPolyDataReader *New();
  vtkTypeRevisionMacro(vtkAbstractPolyDataReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Specify file name for the polydata file. 
  //virtual void SetFileName(const char *);
  //virtual const char *getFileName(void);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
      
  // Description:
  // Return non zero if the reader can read the given file name.
  // Should be implemented by all sub-classes of vtkAbstractPolyDataReader.
  // For non zero return values the following values are to be used
  //   1 - I think I can read the file but I cannot prove it
  //   2 - I definitely can read the file
  //   3 - I can read the file and I have validated that I am the 
  //       correct reader for this file
  virtual int CanReadFile(const char* vtkNotUsed(fname))
    {
    return 0;
    }; 

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) 
    {
    return 0;
    }; 
      
protected:
  vtkAbstractPolyDataReader();
  ~vtkAbstractPolyDataReader();

  char *FileName;

private:
  vtkAbstractPolyDataReader(const vtkAbstractPolyDataReader&);  // Not implemented.
  void operator=(const vtkAbstractPolyDataReader&);  // Not implemented.
};

#endif
