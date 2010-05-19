/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightWriter.h

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

// .NAME vtkEnSightWriter - write vtk unstructured grid data as an EnSight file
// .SECTION Description
// vtkEnSightWriter is a source object that writes binary 
// unstructured grid data files in EnSight format. See EnSight Manual for 
// format details

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// Be sure to specify the endian-ness of the file when reading it into EnSight

#ifndef __vtkEnSightWriter_h
#define __vtkEnSightWriter_h

#include "vtkWriter.h"

class vtkUnstructuredGrid;
class vtkModelMetadata;

class VTK_PARALLEL_EXPORT vtkEnSightWriter : public vtkWriter
{

public:
  vtkTypeMacro(vtkEnSightWriter,vtkWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  //
  static vtkEnSightWriter *New();

  //Description
  //Specify which process this writer is
  vtkSetMacro(ProcessNumber,int);
  vtkGetMacro(ProcessNumber,int);

  // Description:
  // Specify path of EnSight data files to write.
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);

  // Description:
  // Specify base name of EnSight data files to write.
  vtkSetStringMacro(BaseName);
  vtkGetStringMacro(BaseName);

  // Description:
  // Specify the path and base name of the output files.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description
  // Specify the Timestep that this data is for
  vtkSetMacro(TimeStep,int);
  vtkGetMacro(TimeStep,int);

  // Description
  // Specify the number of ghost levels to include in output files
  vtkSetMacro(GhostLevel,int);
  vtkGetMacro(GhostLevel,int);

  // Description
  // Specify whether the geoemtry changes each timestep
  // if false, geometry is only written at timestep 0
  vtkSetMacro(TransientGeometry,bool);
  vtkGetMacro(TransientGeometry,bool);

  //Description
  //set the number of block ID's
  vtkSetMacro(NumberOfBlocks,int);
  vtkGetMacro(NumberOfBlocks,int);

  //Description
  //set the array of Block ID's
  //this class keeps a reference to the array and will not delete it
  virtual void SetBlockIDs(int* val)
    {
    BlockIDs=val;
    }
  virtual int* GetBlockIDs()
    {
    return BlockIDs;
    }

  // Description:
  // Specify the input data or filter.
  virtual void SetInput(vtkUnstructuredGrid *input);
  virtual vtkUnstructuredGrid* GetInput();

  // Description
  // Writes the case file that EnSight is capable of reading
  // The other data files must be written before the case file
  // and the input must be one of the time steps
  // variables must be the same for all time steps or the case file will be 
  // missing variables
  virtual void WriteCaseFile(int TotalTimeSteps);
  virtual void WriteSOSCaseFile(int NumProcs);

  // Description
  // sets and gets the metadata
  virtual void SetModelMetadata(vtkModelMetadata* model);
  vtkGetObjectMacro(ModelMetadata, vtkModelMetadata);

protected:
  vtkEnSightWriter();
  virtual ~vtkEnSightWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual void WriteData(); // method to allow this class to be instantiated and delegated to

  virtual void WriteStringToFile(const char* string, FILE* file);
  virtual void WriteTerminatedStringToFile(const char* string, FILE* file);
  virtual void WriteIntToFile(const int i,FILE* file);
  virtual void WriteFloatToFile(const float f,FILE* file);
  virtual void WriteElementTypeToFile(int ElementType, FILE* fd);

  virtual bool ShouldWriteGeometry();
  virtual void SanitizeFileName(char* name);
  virtual FILE* OpenFile(char* name);

  void ComputeNames();
  void DefaultNames();

  int GetExodusModelIndex(int *ElementArray,int NumberElements,int PartID);

  char *Path;
  char *BaseName;
  char *FileName;
  int TimeStep;
  int GhostLevelMultiplier;
  int ProcessNumber;
  int NumberOfProcesses;
  int NumberOfBlocks;
  int * BlockIDs;
  bool TransientGeometry;
  int GhostLevel;
  vtkUnstructuredGrid* TmpInput;
  vtkModelMetadata* ModelMetadata;

  vtkEnSightWriter(const vtkEnSightWriter&);  // Not implemented.
  void operator=(const vtkEnSightWriter&);  // Not implemented.

};

#endif
