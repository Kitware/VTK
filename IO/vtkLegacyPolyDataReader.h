/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegacyPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLegacyPolyDataReader - read .vtk polydata files
// .SECTION Description
// vtkLegacyPolyDataReader is a source object that reads polydata
// files in .vtk format. The FileName must be specified 
// but is managed by base class vtkAbstractPolyDataReader.  

// .SECTION Caveats

#ifndef __vtkLegacyPolyDataReader_h
#define __vtkLegacyPolyDataReader_h

#include "vtkAbstractPolyDataReader.h"
#include "vtkSmartPointer.h" // required for sp ivars 

class vtkPolyDataReader;

class VTK_IO_EXPORT vtkLegacyPolyDataReader : public vtkAbstractPolyDataReader 
{
public:
  vtkTypeRevisionMacro(vtkLegacyPolyDataReader,vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkLegacyPolyDataReader *New();

  //Invoke public methods of vtkPolyDataReader member variable.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
      
  void SetFileName(const char *filename); 
  char* GetFileName(void); 

protected:
  vtkLegacyPolyDataReader();
  ~vtkLegacyPolyDataReader();
      
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  int CanReadFile(const char *filename); 
      
  //Invoke protected methods of vtkPolyDataReader member variable.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  
  int FillOutputPortInformation(int, vtkInformation*);
      
private:
  vtkLegacyPolyDataReader(const vtkLegacyPolyDataReader&);  // Not implemented.
  void operator=(const vtkLegacyPolyDataReader&);  // Not implemented.
      
 //BTX
 vtkSmartPointer<vtkPolyDataReader> PolyDataReaderPointer;
 //ETX      
};

#endif
