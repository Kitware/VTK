/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetReader.h
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
// .NAME vtkDataSetReader - class to read any type of vtk dataset
// .SECTION Description
// vtkDataSetReader is a class that provides instance variables and methods
// to read any type of dataset in Visualization Toolkit (vtk) format.  The
// output type of this class will vary depending upon the type of data
// file. Convenience methods are provided to keep the data as a particular
// type. (See text for format description details).
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkDataReader vtkPolyDataReader vtkRectilinearGridReader 
// vtkStructuredPointsReader vtkStructuredGridReader vtkUnstructuredGridReader

#ifndef __vtkDataSetReader_h
#define __vtkDataSetReader_h

#include "vtkDataReader.h"
#include "vtkDataSet.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTK_IO_EXPORT vtkDataSetReader : public vtkDataReader
{
public:
  static vtkDataSetReader *New();
  vtkTypeRevisionMacro(vtkDataSetReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source as a general vtkDataSet. Since we need 
  // to know the type of the data, the FileName must be set before GetOutput 
  // is applied.
  vtkDataSet *GetOutput();
  vtkDataSet *GetOutput(int idx)
    {return static_cast<vtkDataSet *>(this->vtkSource::GetOutput(idx)); };

  // Description:
  // Get the output as various concrete types. This method is typically used
  // when you know exactly what type of data is being read.  Otherwise, use
  // the general GetOutput() method. If the wrong type is used NULL is
  // returned.  (You must also set the filename of the object prior to
  // getting the output.)
  vtkPolyData *GetPolyDataOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();

  // Description:
  // If there is no output, execute anyway.  Execute creates an output.
  void Update();
  
protected:
  vtkDataSetReader();
  ~vtkDataSetReader();

  void Execute();
  vtkDataReader *Reader;

private:
  vtkDataSetReader(const vtkDataSetReader&);  // Not implemented.
  void operator=(const vtkDataSetReader&);  // Not implemented.
};

#endif


