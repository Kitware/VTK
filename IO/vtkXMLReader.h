/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLReader - Superclass for VTK's XML format readers.
// .SECTION Description
// vtkXMLReader uses vtkXMLDataParser to parse a VTK XML input file.
// Concrete subclasses then traverse the parsed file structure and
// extract data.

#ifndef __vtkXMLReader_h
#define __vtkXMLReader_h

#include "vtkSource.h"

class vtkCallbackCommand;
class vtkDataArray;
class vtkDataArraySelection;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkXMLDataElement;
class vtkXMLDataParser;

class VTK_IO_EXPORT vtkXMLReader : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkXMLReader,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);
  
  // Description:
  // Get the output as a vtkDataSet pointer.
  vtkDataSet* GetOutputAsDataSet();
  vtkDataSet* GetOutputAsDataSet(int index);
  
  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  
  // Description:  
  // Get the number of point or cell arrays available in the input.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  
  // Description:
  // Get the name of the point or cell array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  
  // Description:
  // Get/Set whether the point or cell array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);  
  void SetCellArrayStatus(const char* name, int status);  
  
protected:
  vtkXMLReader();
  ~vtkXMLReader();
  
  // Standard pipeline exectution methods.
  void ExecuteInformation();
  void ExecuteData(vtkDataObject* output);
  
  // Pipeline execution methods to be defined by subclass.  Called by
  // corresponding Execute methods after appropriate setup has been
  // done.
  virtual void ReadXMLInformation();
  virtual void ReadXMLData();
  
  // Get the name of the data set being read.
  virtual const char* GetDataSetName()=0;
  
  // Test if the reader can read a file with the given version number.
  virtual int CanReadFileVersion(int major, int minor);
  
  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput()=0;
  
  // Setup the output's information and data without allocation.
  virtual void SetupOutputInformation();
  
  // Setup the output's information and data with allocation.
  virtual void SetupOutputData();
  
  // Read the primary element from the file.  This is the element
  // whose name is the value returned by GetDataSetName().
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  
  // Read the top-level element from the file.  This is always the
  // VTKFile element.
  int ReadVTKFile(vtkXMLDataElement* eVTKFile);  
  
  // Create a vtkDataArray from its cooresponding XML representation.
  // Does not allocate.
  vtkDataArray* CreateDataArray(vtkXMLDataElement* da);
  
  // Internal utility methods.
  int OpenVTKFile();
  void CloseVTKFile();
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  void SetupCompressor(const char* type);
  int CanReadFileVersionString(const char* version);
  
  // Utility methods for subclasses.
  int IntersectExtents(int* extent1, int* extent2, int* result);
  int Min(int a, int b);
  int Max(int a, int b);
  void ComputeDimensions(int* extent, int* dimensions, int isPoint);
  void ComputeIncrements(int* extent, int* increments, int isPoint);
  unsigned int GetStartTuple(int* extent, int* increments,
                             int i, int j, int k);
  void ReadAttributeIndices(vtkXMLDataElement* eDSA,
                            vtkDataSetAttributes* dsa);
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);  
  
  // Setup the data array selections for the input's set of arrays.
  void SetDataArraySelections(vtkXMLDataElement* eDSA,
                              vtkDataArraySelection* sel);
  
  // Check whether the given array element is an enabled array.
  int PointDataArrayIsEnabled(vtkXMLDataElement* ePDA);
  int CellDataArrayIsEnabled(vtkXMLDataElement* eCDA);
  
  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject* caller, unsigned long eid,
                                        void* clientdata, void* calldata);
  
  // The vtkXMLDataParser instance used to hide XML reading details.
  vtkXMLDataParser* XMLParser;
  
  // The input file's name.
  char* FileName;
  
  // The stream used to read the input.
  istream* Stream;
  
  // The array selections.
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  
  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;
  
  // Whether there was an error reading the file in ExecuteInformation.
  int InformationError;
  
  // Whether there was an error reading the file in ExecuteData.
  int DataError;
  
  // The index of the output on which ExecuteData is currently
  // running.
  int CurrentOutput;
  
  // The current range over which progress is moving.  This allows for
  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float* range);
  virtual void SetProgressRange(float* range, int curStep, int numSteps);
  virtual void SetProgressRange(float* range, int curStep, float* fractions);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

private:
  // The stream used to read the input if it is in a file.
  ifstream* FileStream;  
  
private:
  vtkXMLReader(const vtkXMLReader&);  // Not implemented.
  void operator=(const vtkXMLReader&);  // Not implemented.
};

#endif
