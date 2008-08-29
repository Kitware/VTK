/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCompositeDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCompositeDataWriter - Writer for multi-group datasets
// .SECTION Description
// vtkXMLCompositeDataWriter writes (serially) the VTK XML multi-group,
// multi-block hierarchical and hierarchical box files. XML multi-group
// data files are meta-files that point to a list of serial VTK XML files.
// .SECTION See Also
// vtkXMLPCompositeDataWriter

#ifndef __vtkXMLCompositeDataWriter_h
#define __vtkXMLCompositeDataWriter_h

#include "vtkXMLWriter.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkCallbackCommand;
class vtkCompositeDataSet;
class vtkXMLDataElement;
class vtkXMLCompositeDataWriterInternals;

class VTK_IO_EXPORT vtkXMLCompositeDataWriter : public vtkXMLWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLCompositeDataWriter,vtkXMLWriter);
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
  vtkXMLCompositeDataWriter();
  ~vtkXMLCompositeDataWriter();

  // Methods to define the file's major and minor version numbers.
  // Major version incremented since v0.1 composite data readers cannot read 
  // the files written by this new reader.
  virtual int GetDataSetMajorVersion() { return 1; }
  virtual int GetDataSetMinorVersion() { return 0; }

  // Create a filename for the given index.
  vtkStdString CreatePieceFileName(int index);
  
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

  // Description:
  // Determine the data types for each of the leaf nodes.
  virtual void FillDataTypes(vtkCompositeDataSet*);

  unsigned int GetNumberOfDataTypes();
  int* GetDataTypesPointer();

  // Methods to create the set of writers matching the set of inputs.
  void CreateWriters(vtkCompositeDataSet*);
  vtkXMLWriter* GetWriter(int index);
  
  // Methods to help construct internal file names.
  void SplitFileName();
  const char* GetFilePrefix();
  const char* GetFilePath();

  // Description:
  // Write the collection file if it is requested.
  // This is overridden in parallel writers to comminitate the hierarchy to the
  // root which then write the meta file.
  int WriteMetaFileIfRequested();
  
  // Make a directory.
  void MakeDirectory(const char* name);
  
  // Remove a directory.
  void RemoveADirectory(const char* name);
  
  // Internal implementation details.
  vtkXMLCompositeDataWriterInternals* Internal;  
  
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

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData, 
    vtkXMLDataElement* element, int &writerIdx)=0;

  // Internal method to write non vtkCompositeDataSet subclass.
  virtual int WriteNonCompositeData(vtkDataObject* dObj, 
    vtkXMLDataElement* element, int& writerIdx);
private:
  vtkXMLCompositeDataWriter(const vtkXMLCompositeDataWriter&);  // Not implemented.
  void operator=(const vtkXMLCompositeDataWriter&);  // Not implemented.
};

#endif
