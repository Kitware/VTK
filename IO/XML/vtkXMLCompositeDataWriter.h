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

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkCallbackCommand;
class vtkCompositeDataSet;
class vtkXMLDataElement;
class vtkXMLCompositeDataWriterInternals;

class VTKIOXML_EXPORT vtkXMLCompositeDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLCompositeDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension();

  // Description:
  // Get/Set the number of pieces into which the inputs are split.

  // Description:
  // Get/Set the number of ghost levels to be written.
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);

  // Description:
  // Get/Set whether this instance will write the meta-file.
  vtkGetMacro(WriteMetaFile, int);
  virtual void SetWriteMetaFile(int flag);

  // Description:
  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkXMLCompositeDataWriter();
  ~vtkXMLCompositeDataWriter();

  // Description:
  // Methods to define the file's major and minor version numbers.
  // Major version incremented since v0.1 composite data readers cannot read
  // the files written by this new reader.
  virtual int GetDataSetMajorVersion() { return 1; }
  virtual int GetDataSetMinorVersion() { return 0; }

  // Description:
  // Create a filename for the given index.
  vtkStdString CreatePieceFileName(int Piece);

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

  // Description:
  // Returns the number of leaf nodes (also includes empty leaf nodes).
  unsigned int GetNumberOfDataTypes();

  // Description:
  // Returns the array pointer to the array of leaf nodes.
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
  // This is overridden in parallel writers to communicate the hierarchy to the
  // root which then write the meta file.
  int WriteMetaFileIfRequested();

  // Make a directory.
  void MakeDirectory(const char* name);

  // Remove a directory.
  void RemoveADirectory(const char* name);

  // Internal implementation details.
  vtkXMLCompositeDataWriterInternals* Internal;

  // The number of ghost levels to write for unstructured data.
  int GhostLevel;

  // Description:
  // Whether to write the collection file on this node. This could
  // potentially be set to 0 (i.e. do not write) for optimization
  // if the file structured does not change but the data does.
  int WriteMetaFile;

  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                       void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;

  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);

  // Description:
  // Internal method called recursively to create the xml tree for
  // the children of compositeData as well as write the actual data
  // set files.  element will only have added nested information.
  // writerIdx is the global piece index used to create unique
  // filenames for each file written.
  // This function returns 0 if no files were written from
  // compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* element, int &writerIdx)=0;

  // Description:
  // Internal method to write a non vtkCompositeDataSet subclass as
  // well as add in the file name to the metadata file.
  // Element is the containing XML metadata element that may
  // have data overwritten and added to (the index XML attribute
  // should not be touched though).  writerIdx is the piece index
  // that gets incremented for the globally numbered piece.
  // This function returns 0 if no file was written (not necessarily an error).
  // this->ErrorCode is set on error.
  virtual int WriteNonCompositeData(
    vtkDataObject* dObj, vtkXMLDataElement* element,
    int& writerIdx, const char* FileName);

  // Description:
  // Utility function to remove any already written files
  // in case writer failed.
  virtual void RemoveWrittenFiles(const char* SubDirectory);

private:
  vtkXMLCompositeDataWriter(const vtkXMLCompositeDataWriter&);  // Not implemented.
  void operator=(const vtkXMLCompositeDataWriter&);  // Not implemented.
};

#endif
