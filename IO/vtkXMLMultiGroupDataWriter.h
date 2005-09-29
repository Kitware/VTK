/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiGroupDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMultiGroupDataWriter - Writer for hierarchical datasets
// .SECTION Description
// vtkXMLMultiGroupDataWriter writes (serially) the VTK XML hierarchical
// and hierarchical box files. XML hierarchical data files are meta-files
// that point to a list of serial VTK XML files.
// .SECTION See Also
// vtkXMLPMultiGroupDataWriter

#ifndef __vtkXMLMultiGroupDataWriter_h
#define __vtkXMLMultiGroupDataWriter_h

#include "vtkXMLWriter.h"

class vtkCallbackCommand;
class vtkMultiGroupDataSet;
class vtkXMLMultiGroupDataWriterInternals;

class VTK_IO_EXPORT vtkXMLMultiGroupDataWriter : public vtkXMLWriter
{
public:
  static vtkXMLMultiGroupDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLMultiGroupDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension();
  
  // Description:
  // Get/Set the piece number to write.  The same piece number is used
  // for all inputs.
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  
  // Description:
  // Get/Set the number of pieces into which the inputs are split.
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set the number of ghost levels to be written.
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);
  
  // Description:
  // Get/Set whether this instance will write the meta-file. 
  vtkGetMacro(WriteMetaFile, int);
  virtual void SetWriteMetaFile(int flag);

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkXMLMultiGroupDataWriter();
  ~vtkXMLMultiGroupDataWriter();
  
  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(
    vtkInformation*  , vtkInformationVector** , vtkInformationVector*);
  int RequestUpdateExtent(
    vtkInformation*  , vtkInformationVector** , vtkInformationVector*);

  virtual int WriteData();
  virtual const char* GetDataSetName();

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  vtkInformation* InputInformation;

  virtual void FillDataTypes(vtkMultiGroupDataSet*);

  unsigned int GetNumberOfDataTypes();
  int* GetDataTypesPointer();

  // Methods to create the set of writers matching the set of inputs.
  void CreateWriters(vtkMultiGroupDataSet*);
  vtkXMLWriter* GetWriter(int index);
  
  // Methods to help construct internal file names.
  void SplitFileName();
  const char* GetFilePrefix();
  const char* GetFilePath();

  // Methods to construct the list of entries for the collection file.
  void AppendEntry(const char* entry);
  void DeleteAllEntries();
  
  // Write the collection file if it is requested.
  int WriteMetaFileIfRequested();
  
  // Make a directory.
  void MakeDirectory(const char* name);
  
  // Remove a directory.
  void RemoveADirectory(const char* name);
  
  // Internal implementation details.
  vtkXMLMultiGroupDataWriterInternals* Internal;  
  
  // The piece number to write.
  int Piece;
  
  // The number of pieces into which the inputs are split.
  int NumberOfPieces;
  
  // The number of ghost levels to write for unstructured data.
  int GhostLevel;
  
  // Whether to write the collection file on this node.
  int WriteMetaFile;
  int WriteMetaFileInitialized;
  
  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                       void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);
  
  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;  
  
  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkXMLMultiGroupDataWriter(const vtkXMLMultiGroupDataWriter&);  // Not implemented.
  void operator=(const vtkXMLMultiGroupDataWriter&);  // Not implemented.
};

#endif
