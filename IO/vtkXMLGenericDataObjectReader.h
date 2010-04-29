/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLGenericDataObjectReader - Read VTK XML HyperOctree files.
// .SECTION Description
// vtkXMLGenericDataObjectReader reads the VTK XML HyperOctree file
// format.  One rectilinear grid file can be read to produce one
// output.  Streaming is supported.  The standard extension for this
// reader's file format is "vto".  This reader is also used to read a
// single piece of the parallel file format.

// .SECTION See Also
// vtkXMLPHyperOctreeReader

#ifndef __vtkXMLGenericDataObjectReader_h
#define __vtkXMLGenericDataObjectReader_h

#include "vtkXMLDataReader.h"

class vtkHierarchicalBoxDataSet;
class vtkHyperOctree;
class vtkMultiBlockDataSet;
class vtkImageData;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_IO_EXPORT vtkXMLGenericDataObjectReader : public vtkXMLDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLGenericDataObjectReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLGenericDataObjectReader *New();

  // Description:
  // Get the reader's output.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx);

  // Description:
  // Get the output as various concrete types. This method is typically used
  // when you know exactly what type of data is being read.  Otherwise, use
  // the general GetOutput() method. If the wrong type is used NULL is
  // returned.  (You must also set the filename of the object prior to
  // getting the output.)
  vtkHierarchicalBoxDataSet *GetHierarchicalBoxDataSetOutput();
  vtkHyperOctree *GetHyperOctreeOutput();
  vtkImageData *GetImageDataOutput();
  vtkMultiBlockDataSet *GetMultiBlockDataSetOutput();
  vtkPolyData *GetPolyDataOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Overridden method.
  vtkIdType GetNumberOfPoints();

  // Description:
  // Overridden method.
  vtkIdType GetNumberOfCells();

  // Description:
  // Overridden method. Not used. Return "vtkDataObject".
  const char* GetDataSetName();

  // Description:
  // Overridden method. Not Used. Delegated.
  void SetupEmptyOutput();

  // Description:
  // This method can be used to find out the type of output expected without
  // needing to read the whole file.
  virtual int ReadOutputType(const char *name, bool &parallel);

protected:
  vtkXMLGenericDataObjectReader();
  ~vtkXMLGenericDataObjectReader();

  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillOutputPortInformation(int, vtkInformation *);

  vtkXMLReader *Reader; // actual reader

private:
  vtkXMLGenericDataObjectReader(const vtkXMLGenericDataObjectReader&);  // Not implemented.
  void operator=(const vtkXMLGenericDataObjectReader&);  // Not implemented.
};

#endif
