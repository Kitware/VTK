/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleReader.h
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
// .NAME vtkParticleReader - Read raw particle data and one array.
// .SECTION Description
// vtkParticleReader reads a raw file with particles.  It supports
// random access into the file to read pieces.  The format is:
// x, y, z, value (all floats).  This class was developed with a 
// specific file in mind, but may be made more general in the future.
// Here are a couple of features I am considering:  
// any data types could be used,
// arbitrary list of arrays to read (extended to vectors as well),
// and time serries based on file names.

#ifndef __vtkParticleReader_h
#define __vtkParticleReader_h

#include "vtkPolyDataSource.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_IO_EXPORT vtkParticleReader : public vtkPolyDataSource
{
public:
  static vtkParticleReader *New();
  vtkTypeRevisionMacro(vtkParticleReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Specify file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // These methods should be used instead of the SwapBytes methods.
  // They indicate the byte ordering of the file you are trying
  // to read in. These methods will then either swap or not swap
  // the bytes depending on the byte ordering of the machine it is
  // being run on. For example, reading in a BigEndian file on a
  // BigEndian machine will result in no swapping. Trying to read
  // the same file on a LittleEndian machine will result in swapping.
  // As a quick note most UNIX machines are BigEndian while PC's
  // and VAX tend to be LittleEndian. So if the file you are reading
  // in was generated on a VAX or PC, SetDataByteOrderToLittleEndian 
  // otherwise SetDataByteOrderToBigEndian. 
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();

  // Description:
  // Set/Get the byte swapping to explicitly swap the bytes of a file.
  vtkSetMacro(SwapBytes,int);
  int GetSwapBytes() {return this->SwapBytes;}
  vtkBooleanMacro(SwapBytes,int);

protected:
  vtkParticleReader();
  ~vtkParticleReader();

  void OpenFile();

  char *FileName;
  ifstream *File;
  int SwapBytes;

  unsigned long NumberOfPoints;
  
  void ExecuteInformation();
  void Execute();
private:
  vtkParticleReader(const vtkParticleReader&);  // Not implemented.
  void operator=(const vtkParticleReader&);  // Not implemented.
};

#endif
