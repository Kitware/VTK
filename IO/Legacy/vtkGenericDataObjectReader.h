/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataObjectReader - class to read any type of vtk data object
// .SECTION Description
// vtkGenericDataObjectReader is a class that provides instance variables and methods
// to read any type of data object in Visualization Toolkit (vtk) format.  The
// output type of this class will vary depending upon the type of data
// file. Convenience methods are provided to return the data as a particular
// type. (See text for format description details).
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkDataReader vtkGraphReader vtkPolyDataReader vtkRectilinearGridReader
// vtkStructuredPointsReader vtkStructuredGridReader vtkTableReader
// vtkTreeReader vtkUnstructuredGridReader

#ifndef __vtkGenericDataObjectReader_h
#define __vtkGenericDataObjectReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkDataObject;
class vtkGraph;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkStructuredPoints;
class vtkTable;
class vtkTree;
class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkGenericDataObjectReader : public vtkDataReader
{
public:
  static vtkGenericDataObjectReader *New();
  vtkTypeMacro(vtkGenericDataObjectReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this filter
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx);

  // Description:
  // Get the output as various concrete types. This method is typically used
  // when you know exactly what type of data is being read.  Otherwise, use
  // the general GetOutput() method. If the wrong type is used NULL is
  // returned.  (You must also set the filename of the object prior to
  // getting the output.)
  vtkGraph *GetGraphOutput();
  vtkPolyData *GetPolyDataOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkTable *GetTableOutput();
  vtkTree *GetTreeOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // This method can be used to find out the type of output expected without
  // needing to read the whole file.
  virtual int ReadOutputType();

  // Description:
  // See vtkAlgorithm for information.
  virtual int ProcessRequest(vtkInformation *, vtkInformationVector **,
                             vtkInformationVector *);
//BTX
protected:
  vtkGenericDataObjectReader();
  ~vtkGenericDataObjectReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *);
  virtual int FillOutputPortInformation(int, vtkInformation *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

private:
  vtkGenericDataObjectReader(const vtkGenericDataObjectReader&);  // Not implemented.
  void operator=(const vtkGenericDataObjectReader&);  // Not implemented.

  template<typename ReaderT, typename DataT>
    void ReadData(const char* dataClass, vtkDataObject* output);

  vtkSetStringMacro(Header);
//ETX
};

#endif
