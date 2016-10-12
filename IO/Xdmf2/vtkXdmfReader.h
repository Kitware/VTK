/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfReader.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXdmfReader
 * @brief   Reads <tt>eXtensible Data Model and Format</tt> files
 *
 * vtkXdmfReader reads XDMF data files so that they can be visualized using
 * VTK. The output data produced by this reader depends on the number of grids
 * in the data file. If the data file has a single domain with a single grid,
 * then the output type is a vtkDataSet subclass of the appropriate type,
 * otherwise it's a vtkMultiBlockDataSet.
 *
 * Refer to vtkDataReader which provides many methods for controlling the
 * reading of the data file.
 * @warning
 * Uses the XDMF API (http://www.xdmf.org)
 * @sa
 * vtkDataReader
*/

#ifndef vtkXdmfReader_h
#define vtkXdmfReader_h

#include "vtkIOXdmf2Module.h" // For export macro
#include "vtkDataReader.h"
#include <map> // for caching

class vtkXdmfArraySelection;
class vtkXdmfDocument;
class vtkGraph;

class VTKIOXDMF2_EXPORT vtkXdmfReader : public vtkDataReader
{
public:
  static vtkXdmfReader* New();
  vtkTypeMacro(vtkXdmfReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Until needed, multiple domains are not supported.
  //// Description:
  //// Returns the number of domains present in the data file. This in valid after
  //// the filename has been set and UpdateInformation() has been called .i.e. the
  //// RequestInformation pipeline pass has happened.
  //unsigned int GetNumberOfDomains();

  //@{
  /**
   * Set the active domain. Only one domain can be selected at a time. By
   * default the first domain in the datafile is chosen. Setting this to null
   * results in the domain being automatically chosen. Note that if the domain
   * name is changed, you should explicitly call UpdateInformation() before
   * accessing information about grids, data arrays etc.
   */
  vtkSetStringMacro(DomainName);
  vtkGetStringMacro(DomainName);
  //@}

  //// Description:
  //// Returns the name for the active domain. Note that this may be different
  //// from what GetDomainName() returns if DomainName is NULL or invalid.
  // vtkGetStringMacro(ActiveDomainName);

  /**
   * Get information about point-based arrays. As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfPointArrays();

  /**
   * Returns the name of point array at the give index. Returns NULL if index is
   * invalid.
   */
  const char* GetPointArrayName(int index);

  //@{
  /**
   * Get/Set the point array status.
   */
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Get information about cell-based arrays.  As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  void SetCellArrayStatus(const char* name, int status);
  int GetCellArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Get/Set information about grids. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfGrids();
  const char* GetGridName(int index);
  void SetGridStatus(const char* gridname, int status);
  int GetGridStatus(const char* gridname);
  //@}

  //@{
  /**
   * Get/Set information about sets. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called. Note that sets with non-zero Ghost value are not treated as sets
   * that the user can select using this API.
   */
  int GetNumberOfSets();
  const char* GetSetName(int index);
  void SetSetStatus(const char* gridname, int status);
  int GetSetStatus(const char* gridname);
  //@}

  /**
   * These methods are provided to make it easier to use the Sets in ParaView.
   */
  int GetNumberOfSetArrays() { return this->GetNumberOfSets(); }
  const char* GetSetArrayName(int index)
    { return this->GetSetName(index); }
  int GetSetArrayStatus(const char* name)
    { return this->GetSetStatus(name); }

  //@{
  /**
   * Get/Set the stride used to skip points when reading structured datasets.
   * This affects all grids being read.
   */
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);
  //@}

  /**
   * Determine if the file can be read with this reader.
   */
  virtual int CanReadFile(const char* filename);

  //@{
  /**
   * Every time the SIL is updated a this will return a different value.
   */
  vtkGetMacro(SILUpdateStamp, int);
  //@}

  /**
   * SIL describes organization of/relationships between classifications
   * eg. blocks/materials/hierarchies.
   */
  virtual vtkGraph* GetSIL();

  class XdmfDataSetTopoGeoPath
  {
  public:
    XdmfDataSetTopoGeoPath() : dataset(0), topologyPath(), geometryPath() {}
    vtkDataSet* dataset;
    std::string topologyPath;
    std::string geometryPath;
  };

  typedef std::map<int, XdmfDataSetTopoGeoPath> XdmfReaderCachedData;

  /**
   * Get the data set cache
   */
  XdmfReaderCachedData& GetDataSetCache();

protected:
  vtkXdmfReader();
  ~vtkXdmfReader();

  virtual int ProcessRequest(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);
  virtual int RequestDataObject(vtkInformationVector *outputVector);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  vtkXdmfArraySelection* GetPointArraySelection();
  vtkXdmfArraySelection* GetCellArraySelection();
  vtkXdmfArraySelection* GetGridSelection();
  vtkXdmfArraySelection* GetSetsSelection();
  void PassCachedSelections();

  char* DomainName;
  // char* ActiveDomainName;
  int Stride[3];
  unsigned int LastTimeIndex;

  vtkXdmfDocument* XdmfDocument;

  // Until RequestInformation() is called, the active domain is not set
  // correctly. If SetGridStatus() etc. are called before that happens, then we
  // have no place to save the user choices. So we cache them in these temporary
  // caches. These are passed on to the actual vtkXdmfArraySelection instances
  // used by the active vtkXdmfDomain in RequestInformation().
  // Note that these are only used until the first domain is setup, once that
  // happens, the information set in these is passed to the domain and  these
  // are cleared an no longer used, until the active domain becomes invalid
  // again.
  vtkXdmfArraySelection* PointArraysCache;
  vtkXdmfArraySelection* CellArraysCache;
  vtkXdmfArraySelection* GridsCache;
  vtkXdmfArraySelection* SetsCache;

  int SILUpdateStamp;

  XdmfReaderCachedData DataSetCache;

private:
  /**
   * Prepares the XdmfDocument.
   */
  bool PrepareDocument();

  void ClearDataSetCache();

  /**
   * Returns the time-step index requested using the UPDATE_TIME_STEPS from the
   * information.
   */
  int ChooseTimeStep(vtkInformation* outInfo);

private:
  vtkXdmfReader(const vtkXdmfReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXdmfReader&) VTK_DELETE_FUNCTION;

};

#endif
