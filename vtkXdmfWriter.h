/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkXdmfWriter - write eXtensible Data Model and Format files
// .SECTION Description
// vtkXdmfWriter converts vtkDataObjects to XDMF format. This is intended to
// replace vtkXdmfWriter, which is not up to date with the capabilities of the
// newer XDMF2 library. This writer understands VTK's composite data types and
// produces full trees in the output XDMF files.

#ifndef _vtkXdmfWriter_h
#define _vtkXdmfWriter_h

#include "vtkIOXdmf2Module.h" // For export macro

#include "vtkDataObjectAlgorithm.h"

class vtkExecutive;

class vtkCompositeDataSet;
class vtkDataArray;
class vtkDataSet;
class vtkDataObject;
class vtkFieldData;
class vtkInformation;
class vtkInformationVector;
class vtkXdmfWriterDomainMemoryHandler;
class XdmfArray;
class XdmfDOM;
class XdmfGrid;

class VTKIOXDMF2_EXPORT vtkXdmfWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkXdmfWriter *New();
  vtkTypeMacro(vtkXdmfWriter,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input data set.
  virtual void SetInputData(vtkDataObject* dobj);

  // Description:
  // Set or get the file name of the xdmf file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set or get the file name of the hdf5 file.
  // Note that if the File name is not specified, then the group name is ignore
  vtkSetStringMacro(HeavyDataFileName);
  vtkGetStringMacro(HeavyDataFileName);

  // Description:
  // Set or get the group name into which data will be written
  // it may contain nested groups as in "/Proc0/Block0"
  vtkSetStringMacro(HeavyDataGroupName);
  vtkGetStringMacro(HeavyDataGroupName);

  // Description:
  // Write data to output. Method executes subclasses WriteData() method, as
  // well as StartMethod() and EndMethod() methods.
  // Returns 1 on success and 0 on failure.
  virtual int Write();

  // Description:
  // Topology Geometry and Attribute arrays smaller than this are written in line into the XML.
  // Default is 100.
  vtkSetMacro(LightDataLimit, int);
  vtkGetMacro(LightDataLimit, int);

  //Description:
  //Controls whether writer automatically writes all input time steps, or
  //just the timestep that is currently on the input.
  //Default is OFF.
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);

    // Description:
  // Called in parallel runs to identify the portion this process is responsible for
  // TODO: respect this
  vtkSetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);

  //TODO: control choice of heavy data format (xml, hdf5, sql, raw)

  //TODO: These controls are available in vtkXdmfWriter, but are not used here.
  //GridsOnly
  //Append to Domain

protected:
  vtkXdmfWriter();
  ~vtkXdmfWriter();

  //Choose composite executive by default for time.
  virtual vtkExecutive* CreateDefaultExecutive();

  //Can take any one data object
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  //Overridden to ...
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);
  //Overridden to ...
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  //Overridden to ...
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  //These do the work: recursively parse down input's structure all the way to arrays,
  //use XDMF lib to dump everything to file.

  virtual void CreateTopology(vtkDataSet *ds, XdmfGrid *grid, vtkIdType PDims[3], vtkIdType CDims[3], vtkIdType &PRank, vtkIdType &CRank, void *staticdata);
  virtual void CreateGeometry(vtkDataSet *ds, XdmfGrid *grid, void *staticdata);

  virtual void WriteDataSet(vtkDataObject *dobj, XdmfGrid *grid);
  virtual void WriteCompositeDataSet(vtkCompositeDataSet *dobj, XdmfGrid *grid);
  virtual void WriteAtomicDataSet(vtkDataObject *dobj, XdmfGrid *grid);
  virtual void WriteArrays(vtkFieldData* dsa, XdmfGrid *grid, int association,
                           vtkIdType rank, vtkIdType *dims, const char *name);
  virtual void ConvertVToXArray(vtkDataArray *vda, XdmfArray *xda,
                                vtkIdType rank, vtkIdType *dims,
                                int AllocStrategy, const char *heavyprefix);

  char *FileName;
  char *HeavyDataFileName;
  char *HeavyDataGroupName;

  int LightDataLimit;

  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;

  int Piece;
  int NumberOfPieces;

  XdmfDOM *DOM;
  XdmfGrid *TopTemporalGrid;

  vtkXdmfWriterDomainMemoryHandler *DomainMemoryHandler;

private:
  vtkXdmfWriter(const vtkXdmfWriter&); // Not implemented
  void operator=(const vtkXdmfWriter&); // Not implemented
};

#endif /* _vtkXdmfWriter_h */
