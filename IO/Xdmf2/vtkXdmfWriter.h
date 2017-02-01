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

/**
 * @class   vtkXdmfWriter
 * @brief   write eXtensible Data Model and Format files
 *
 * vtkXdmfWriter converts vtkDataObjects to XDMF format. This is intended to
 * replace vtkXdmfWriter, which is not up to date with the capabilities of the
 * newer XDMF2 library. This writer understands VTK's composite data types and
 * produces full trees in the output XDMF files.
*/

#ifndef vtkXdmfWriter_h
#define vtkXdmfWriter_h

#include "vtkIOXdmf2Module.h" // For export macro

#include "vtkDataObjectAlgorithm.h"

#include <string> // Needed for private members
#include <vector> //

class vtkExecutive;

class vtkCompositeDataSet;
class vtkDataArray;
class vtkDataSet;
class vtkDataObject;
class vtkFieldData;
class vtkInformation;
class vtkInformationVector;
class vtkXdmfWriterDomainMemoryHandler;

namespace xdmf2
{
class XdmfArray;
class XdmfDOM;
class XdmfElement;
class XdmfGrid;
class XdmfGeometry;
class XdmfTopology;
}

class VTKIOXDMF2_EXPORT vtkXdmfWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkXdmfWriter *New();
  vtkTypeMacro(vtkXdmfWriter,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the input data set.
   */
  virtual void SetInputData(vtkDataObject* dobj);

  //@{
  /**
   * Set or get the file name of the xdmf file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set or get the file name of the hdf5 file.
   * Note that if the File name is not specified, then the group name is ignore
   */
  vtkSetStringMacro(HeavyDataFileName);
  vtkGetStringMacro(HeavyDataFileName);
  //@}

  //@{
  /**
   * Set or get the group name into which data will be written
   * it may contain nested groups as in "/Proc0/Block0"
   */
  vtkSetStringMacro(HeavyDataGroupName);
  vtkGetStringMacro(HeavyDataGroupName);
  //@}

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  //@{
  /**
   * Topology Geometry and Attribute arrays smaller than this are written in line into the XML.
   * Default is 100.
   * Node: LightDataLimit is forced to 1 when MeshStaticOverTime is TRUE.
   */
  vtkSetMacro(LightDataLimit, int);
  vtkGetMacro(LightDataLimit, int);
  //@}

  //@{
  /**
   * Controls whether writer automatically writes all input time steps, or
   * just the timestep that is currently on the input.
   * Default is OFF.
   */
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);
  //@}

  //@{
  /**
   * Set of get the flag that specify if input mesh is static over time.
   * If so, the mesh topology and geometry heavy data will be written only once.
   * Default if FALSE.
   * Note: this mode requires that all data is dumped in the heavy data file.
   */
  vtkSetMacro(MeshStaticOverTime, bool);
  vtkGetMacro(MeshStaticOverTime, bool);
  vtkBooleanMacro(MeshStaticOverTime, bool);
  //@}

    //@{
    /**
     * Called in parallel runs to identify the portion this process is responsible for
     * TODO: respect this
     */
  vtkSetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
    //@}

  //TODO: control choice of heavy data format (xml, hdf5, sql, raw)

  //TODO: These controls are available in vtkXdmfWriter, but are not used here.
  //GridsOnly
  //Append to Domain

protected:
  vtkXdmfWriter();
  ~vtkXdmfWriter();

  //Choose composite executive by default for time.
  virtual vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  //Can take any one data object
  virtual int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  //Overridden to ...
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) VTK_OVERRIDE;
  //Overridden to ...
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  //Overridden to ...
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

  //These do the work: recursively parse down input's structure all the way to arrays,
  //use XDMF lib to dump everything to file.

  virtual int CreateTopology(vtkDataSet *ds, xdmf2::XdmfGrid *grid, vtkIdType PDims[3], vtkIdType CDims[3], vtkIdType &PRank, vtkIdType &CRank, void *staticdata);
  virtual int CreateGeometry(vtkDataSet *ds, xdmf2::XdmfGrid *grid, void *staticdata);

  virtual int WriteDataSet(vtkDataObject *dobj, xdmf2::XdmfGrid *grid);
  virtual int WriteCompositeDataSet(vtkCompositeDataSet *dobj, xdmf2::XdmfGrid *grid);
  virtual int WriteAtomicDataSet(vtkDataObject *dobj, xdmf2::XdmfGrid *grid);
  virtual int WriteArrays(vtkFieldData* dsa, xdmf2::XdmfGrid *grid, int association,
                           vtkIdType rank, vtkIdType *dims, const char *name);
  virtual void ConvertVToXArray(vtkDataArray *vda, xdmf2::XdmfArray *xda,
                                vtkIdType rank, vtkIdType *dims,
                                int AllocStrategy, const char *heavyprefix);

  virtual void SetupDataArrayXML(xdmf2::XdmfElement*, xdmf2::XdmfArray*) const;

  char *FileName;
  char *HeavyDataFileName;
  char *HeavyDataGroupName;
  std::string WorkingDirectory;
  std::string BaseFileName;

  int LightDataLimit;

  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  double CurrentTime;
  int CurrentTimeIndex;
  int CurrentBlockIndex;
  int UnlabelledDataArrayId;

  int Piece;
  int NumberOfPieces;

  bool MeshStaticOverTime;

  xdmf2::XdmfDOM *DOM;
  xdmf2::XdmfGrid *TopTemporalGrid;

  vtkXdmfWriterDomainMemoryHandler *DomainMemoryHandler;

  std::vector<xdmf2::XdmfTopology*> TopologyAtT0;
  std::vector<xdmf2::XdmfGeometry*> GeometryAtT0;

private:
  vtkXdmfWriter(const vtkXdmfWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXdmfWriter&) VTK_DELETE_FUNCTION;
};

#endif /* vtkXdmfWriter_h */
