/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayWriter.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkArrayWriter_h
#define __vtkArrayWriter_h

#include <vtkArrayDataAlgorithm.h>

// .NAME vtkArrayWriter - Serialize sparse and dense arrays to a file or stream.
//
// .SECTION Description
// vtkArrayWriter serializes sparse and dense array data using a text-based
// format that is human-readable and easily parsed (default option).  The 
// WriteBinary array option can be set to true in the Write method, which 
// will serialize the sparse and dense array data using a binary format that 
// is optimized for rapid throughput.
//
// Inputs:
//   Input port 0: (required) vtkArrayData object containing a sparse or dense array.
//
// Output Format:
//   The first line of output will contain the array type (sparse or dense) and the
//   type of values stored in the array (double, integer, string, etc).
//
//   The second line of output will contain the array extents along each dimension
//   of the array, followed by the number of non-null values stored in the array.
//
//   For sparse arrays, each subsequent line of output will contain the coordinates
//   and value for each non-null value stored in the array.
//
//   For dense arrays, each subsequent line of output will contain one value from the
//   array, stored in the same order as that used by vtkArrayCoordinateIterator.
//
// .SECTION See Also
// vtkArrayReader
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.


class vtkArray;

class VTK_IO_EXPORT vtkArrayWriter :
  public vtkArrayDataAlgorithm
{
public:
  static vtkArrayWriter *New();
  vtkTypeMacro(vtkArrayWriter, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Write input port 0 data to a file.
  bool Write(const vtkStdString& file_name, bool WriteBinary = false);

  // Description:
  // Write an arbitrary array to a file.
  static bool Write(vtkArray* array, const vtkStdString& file_name, bool WriteBinary = false); 

//BTX
  // Description:
  // Write input port 0 data to a stream.
  bool Write(ostream& stream, bool WriteBinary = false);

  // Description:
  // Write arbitrary data to a stream.
  static bool Write(vtkArray* array, ostream& stream, bool WriteBinary = false);
//ETX

protected:
  vtkArrayWriter();
  ~vtkArrayWriter();

private:
  vtkArrayWriter(const vtkArrayWriter&);  // Not implemented.
  void operator=(const vtkArrayWriter&);  // Not implemented.
};

#endif

