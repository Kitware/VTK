// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkSLACReader
//
// .SECTION Description
//
// A reader for a data format used by Omega3p, Tau3p, and several other tools
// used at the Standford Linear Accelerator Center (SLAC).  The underlying
// format uses netCDF to store arrays, but also imposes several conventions
// to form an unstructured grid of elements.
//

#ifndef vtkSLACReader_h
#define vtkSLACReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include "vtkSmartPointer.h"      // For internal method.

class vtkDataArraySelection;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;

class VTKIONETCDF_EXPORT vtkSLACReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSLACReader, vtkMultiBlockDataSetAlgorithm);
  static vtkSLACReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  vtkGetStringMacro(MeshFileName);
  vtkSetStringMacro(MeshFileName);

  // Description:
  // There may be one mode file (usually for actual modes) or multiple mode
  // files (which usually actually represent time series).  These methods
  // set and clear the list of mode files (which can be a single mode file).
  virtual void AddModeFileName(const char *fname);
  virtual void RemoveAllModeFileNames();
  virtual unsigned int GetNumberOfModeFileNames();
  virtual const char *GetModeFileName(unsigned int idx);

  // Description:
  // If on, reads the internal volume of the data set.  Set to off by default.
  vtkGetMacro(ReadInternalVolume, int);
  vtkSetMacro(ReadInternalVolume, int);
  vtkBooleanMacro(ReadInternalVolume, int);

  // Description:
  // If on, reads the external surfaces of the data set.  Set to on by default.
  vtkGetMacro(ReadExternalSurface, int);
  vtkSetMacro(ReadExternalSurface, int);
  vtkBooleanMacro(ReadExternalSurface, int);

  // Description:
  // If on, reads midpoint information for external surfaces and builds
  // quadratic surface triangles.  Set to on by default.
  vtkGetMacro(ReadMidpoints, int);
  vtkSetMacro(ReadMidpoints, int);
  vtkBooleanMacro(ReadMidpoints, int);

  // Description:
  // Variable array selection.
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);

  // Description:
  // Sets the scale factor for each mode. Each scale factor is reset to 1.
  virtual void ResetFrequencyScales();
  virtual void SetFrequencyScale(int index, double scale);

  // Description:
  // Sets the phase offset for each mode. Each shift is reset to 0.
  virtual void ResetPhaseShifts();
  virtual void SetPhaseShift(int index, double shift);

  // Description:
  // NOTE: This is not thread-safe.
  virtual vtkDoubleArray* GetFrequencyScales();
  virtual vtkDoubleArray* GetPhaseShifts();

  // Description:
  // Returns true if the given file can be read by this reader.
  static int CanReadFile(const char *filename);

  // Description:
  // This key is attached to the metadata information of all data sets in the
  // output that are part of the internal volume.
  static vtkInformationIntegerKey *IS_INTERNAL_VOLUME();

  // Description:
  // This key is attached to the metadata information of all data sets in the
  // output that are part of the external surface.
  static vtkInformationIntegerKey *IS_EXTERNAL_SURFACE();

  // Description:
  // All the data sets stored in the multiblock output share the same point
  // data.  For convienience, the point coordinates (vtkPoints) and point data
  // (vtkPointData) are saved under these keys in the vtkInformation of the
  // output data set.
  static vtkInformationObjectBaseKey *POINTS();
  static vtkInformationObjectBaseKey *POINT_DATA();

//BTX
  // Description:
  // Simple class used internally to define an edge based on the endpoints.  The
  // endpoints are canonically identified by the lower and higher values.
  class VTKIONETCDF_EXPORT EdgeEndpoints
  {
  public:
    EdgeEndpoints() : MinEndPoint(-1), MaxEndPoint(-1) {}
    EdgeEndpoints(vtkIdType endpointA, vtkIdType endpointB) {
      if (endpointA < endpointB)
        {
        this->MinEndPoint = endpointA;  this->MaxEndPoint = endpointB;
        }
      else
        {
        this->MinEndPoint = endpointB;  this->MaxEndPoint = endpointA;
        }
    }
    inline vtkIdType GetMinEndPoint() const { return this->MinEndPoint; }
    inline vtkIdType GetMaxEndPoint() const { return this->MaxEndPoint; }
    inline bool operator==(const EdgeEndpoints &other) const {
      return (   (this->GetMinEndPoint() == other.GetMinEndPoint())
              && (this->GetMaxEndPoint() == other.GetMaxEndPoint()) );
    }
  protected:
    vtkIdType MinEndPoint;
    vtkIdType MaxEndPoint;
  };

  // Description:
  // Simple class used internally for holding midpoint information.
  class VTKIONETCDF_EXPORT MidpointCoordinates
  {
  public:
    MidpointCoordinates() {}
    MidpointCoordinates(const double coord[3], vtkIdType id) {
      this->Coordinate[0] = coord[0];
      this->Coordinate[1] = coord[1];
      this->Coordinate[2] = coord[2];
      this->ID = id;
    }
    double Coordinate[3];
    vtkIdType ID;
  };

  enum {
    SURFACE_OUTPUT = 0,
    VOLUME_OUTPUT = 1,
    NUM_OUTPUTS = 2
  };
//ETX

protected:
  vtkSLACReader();
  ~vtkSLACReader();

//BTX
  class vtkInternal;
  vtkInternal *Internal;

  // Friend so vtkInternal can access MidpointIdMap
  // (so Sun CC compiler doesn't complain).
  friend class vtkInternal;

  char *MeshFileName;

  int ReadInternalVolume;
  int ReadExternalSurface;
  int ReadMidpoints;

  // Description:
  // True if reading from a proper mode file.  Set in RequestInformation.
  bool ReadModeData;

  // Description:
  // True if "mode" files are a sequence of time steps.
  bool TimeStepModes;

  // Description:
  // True if mode files describe vibrating fields.
  bool FrequencyModes;

//ETX

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Description:
  // Callback registered with the VariableArraySelection.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  // Description:
  // Convenience function that checks the dimensions of a 2D netCDF array that
  // is supposed to be a set of tuples.  It makes sure that the number of
  // dimensions is expected and that the number of components in each tuple
  // agree with what is expected.  It then returns the number of tuples.  An
  // error is emitted and 0 is returned if the checks fail.
  virtual vtkIdType GetNumTuplesInVariable(int ncFD, int varId,
                                           int expectedNumComponents);

  // Description:
  // Checks the winding of the tetrahedra in the mesh file.  Returns 1 if
  // the winding conforms to VTK, 0 if the winding needs to be corrected.
  virtual int CheckTetrahedraWinding(int meshFD);

  // Description:
  // Read the connectivity information from the mesh file.  Returns 1 on
  // success, 0 on failure.
  virtual int ReadConnectivity(int meshFD, vtkMultiBlockDataSet *surfaceOutput,
                               vtkMultiBlockDataSet *volumeOutput);

  // Description:
  // Reads tetrahedron connectivity arrays.  Called by ReadConnectivity.
  virtual int ReadTetrahedronInteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity);
  virtual int ReadTetrahedronExteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity);

//BTX
  // Description:
  // Reads point data arrays.  Called by ReadCoordinates and ReadFieldData.
  virtual vtkSmartPointer<vtkDataArray> ReadPointDataArray(int ncFD, int varId);
//ETX

//BTX
  // Description:
  // Helpful constants equal to the amount of identifiers per tet.
  enum {
    NumPerTetInt = 5,
    NumPerTetExt = 9
  };
//ETX

//BTX
  // Description:
  // Manages a map from edges to midpoint coordinates.
  class VTKIONETCDF_EXPORT MidpointCoordinateMap
  {
  public:
    MidpointCoordinateMap();
    ~MidpointCoordinateMap();

    void AddMidpoint(const EdgeEndpoints &edge,
                     const MidpointCoordinates &midpoint);
    void RemoveMidpoint(const EdgeEndpoints &edge);
    void RemoveAllMidpoints();
    vtkIdType GetNumberOfMidpoints() const;

    // Description:
    // Finds the coordinates for the given edge or returns NULL if it
    // does not exist.
    MidpointCoordinates *FindMidpoint(const EdgeEndpoints &edge);

  protected:
    class vtkInternal;
    vtkInternal *Internal;

  private:
    // Too lazy to implement these.
    MidpointCoordinateMap(const MidpointCoordinateMap &);
    void operator=(const MidpointCoordinateMap &);
  };

  // Description:
  // Manages a map from edges to the point id of the midpoint.
  class VTKIONETCDF_EXPORT MidpointIdMap
  {
  public:
    MidpointIdMap();
    ~MidpointIdMap();

    void AddMidpoint(const EdgeEndpoints &edge, vtkIdType midpoint);
    void RemoveMidpoint(const EdgeEndpoints &edge);
    void RemoveAllMidpoints();
    vtkIdType GetNumberOfMidpoints() const;

    // Description:
    // Finds the id for the given edge or returns NULL if it does not exist.
    vtkIdType *FindMidpoint(const EdgeEndpoints &edge);

    // Description:
    // Initialize iteration.  The iteration can occur in any order.
    void InitTraversal();
    // Description:
    // Get the next midpoint in the iteration.  Return 0 if the end is reached.
    bool GetNextMidpoint(EdgeEndpoints &edge, vtkIdType &midpoint);

  protected:
    class vtkInternal;
    vtkInternal *Internal;

  private:
    // Too lazy to implement these.
    MidpointIdMap(const MidpointIdMap &);
    void operator=(const MidpointIdMap &);
  };
//ETX

  // Description:
  // Read in the point coordinate data from the mesh file.  Returns 1 on
  // success, 0 on failure.
  virtual int ReadCoordinates(int meshFD, vtkMultiBlockDataSet *output);

  // Description:
  // Reads in the midpoint coordinate data from the mesh file and returns a map
  // from edges to midpoints.  This method is called by ReadMidpointData.
  // Returns 1 on success, 0 on failure.
  virtual int ReadMidpointCoordinates(int meshFD, vtkMultiBlockDataSet *output,
                                      MidpointCoordinateMap &map);

  // Description:
  // Read in the midpoint data from the mesh file.  Returns 1 on success,
  // 0 on failure.  Also fills a midpoint id map that will be passed into
  // InterpolateMidpointFieldData.
  virtual int ReadMidpointData(int meshFD, vtkMultiBlockDataSet *output,
                               MidpointIdMap &map);

  // Description:
  // Instead of reading data from the mesh file, restore the data from the
  // previous mesh file read.
  virtual int RestoreMeshCache(vtkMultiBlockDataSet *surfaceOutput,
                               vtkMultiBlockDataSet *volumeOutput,
                               vtkMultiBlockDataSet *compositeOutput);

  // Description:
  // Read in the field data from the mode file.  Returns 1 on success, 0
  // on failure.
  virtual int ReadFieldData(const int *modeFDArray,
                            int numModeFDs,
                            vtkMultiBlockDataSet *output);

  // Description:
  // Takes the data read on the fields and interpolates data for the midpoints.
  // map is the same map that was created in ReadMidpointData.
  virtual int InterpolateMidpointData(vtkMultiBlockDataSet *output,
                                      MidpointIdMap &map);

  // Description:
  // A time stamp for the last time the mesh file was read.  This is used to
  // determine whether the mesh needs to be read in again or if we just need
  // to read the mode data.
  vtkTimeStamp MeshReadTime;

  // Description:
  // Returns 1 if the mesh is up to date, 0 if the mesh needs to be read from
  // disk.
  virtual int MeshUpToDate();

private:
  vtkSLACReader(const vtkSLACReader &);         // Not implemented
  void operator=(const vtkSLACReader &);        // Not implemented
};

#endif //vtkSLACReader_h
