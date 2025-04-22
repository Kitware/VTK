// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNek5000Reader
 * @brief   Reads Nek5000 format data files.
 *
 * @par Thanks:
 * This class was developed by  Jean Favre (jfavre@cscs.ch) from
 * the Swiss National Supercomputing Centre
 */

#ifndef vtkNek5000Reader_h
#define vtkNek5000Reader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkPoints;
class vtkDataArraySelection;

class VTKIOPARALLEL_EXPORT vtkNek5000Reader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNek5000Reader* New();
  vtkTypeMacro(vtkNek5000Reader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMTimeType GetMTime() override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetStringMacro(DataFileName);
  vtkGetStringMacro(DataFileName);

  vtkGetMacro(NumberOfTimeSteps, int);
  ///@{
  /**
   * Returns the available range of valid integer time steps.
   */
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetVector2Macro(TimeStepRange, int);
  ///@}

  /**
   * Get the number of point arrays available in the input.
   */
  int GetNumberOfPointArrays();

  /**
   * Get the name of the  point array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  /**
   * used for ParaView to decide if cleaning the grid to merge points
   */
  vtkSetMacro(CleanGrid, int);
  vtkGetMacro(CleanGrid, int);
  vtkBooleanMacro(CleanGrid, int);

  ///@{
  /**
   * used for ParaView to decide if showing the spectral elements ids as cell-data
   */
  vtkSetMacro(SpectralElementIds, int);
  vtkGetMacro(SpectralElementIds, int);
  vtkBooleanMacro(SpectralElementIds, int);
  ///@}

  ///@{
  /**
   * Get/Set whether the point array with the given name or index is to be read.
   */
  bool GetPointArrayStatus(const char* name);
  bool GetPointArrayStatus(int index);
  void SetPointArrayStatus(const char* name, int status);
  ///@}

  ///@{
  /**
   * Turn on/off all point arrays.
   */
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  ///@}

  /**
   * Get the names of variables stored in the data
   */
  size_t GetVariableNamesFromData(char* varTags);

  int CanReadFile(const char* fname);

protected:
  vtkNek5000Reader();
  ~vtkNek5000Reader() override;

  char* FileName;
  char* DataFileName;
  //  int ElementResolution;
  //  int BoundaryResolution;
  int nfields;
  //  int my_patch_id;

  int num_vars; // all vars including Pressure, Velocity, Velocity Magnitude and Temperature
  char** var_names;
  float** dataArray;
  int num_der_vars;

  int* var_length;

  int num_used_scalars;
  int num_used_vectors;

  // Tri* T;
  class nek5KList;
  class nek5KObject;
  nek5KList* myList;
  nek5KObject* curObj;
  int displayed_step;
  int memory_step;
  int requested_step;

  float* meshCoords;

  std::string datafile_format;
  int datafile_start;
  int datafile_num_steps;
  bool* timestep_has_mesh;

  //  void setActive();  // set my_patch_id as the active one
  //  static int getNextPatchID(){return(next_patch_id++);}

  vtkDataArraySelection* PointDataArraySelection;

  // update which fields from the data should be used, based on GUI
  void updateVariableStatus();
  void partitionAndReadMesh();
  void readData(char* dfName);
  // copy the data from nek5000 to pv
  void updateVtuData(vtkUnstructuredGrid* pv_ugrid); //, vtkUnstructuredGrid* pv_boundary_ugrid);
  void addCellsToContinuumMesh();
  void addSpectralElementId(int nelements);
  void copyContinuumPoints(vtkPoints* points);
  // void interpolateAndCopyContinuumData(vtkUnstructuredGrid* pv_ugrid, double **data_array, int
  // interp_res, int num_verts);
  void copyContinuumData(vtkUnstructuredGrid* pv_ugrid);
  //  void interpolateAndCopyBoundaryPoints(int alloc_res, int interp_res, vtkPoints*
  //  boundary_points); void interpolateAndCopyBoundaryData(int alloc_res, int num_verts, int
  //  interp_res); void addCellsToBoundaryMesh(int * boundary_index, int qa); void
  //  generateBoundaryConnectivity(int * boundary_index, int res);
  // see if the current object is missing data that was requested
  bool isObjectMissingData();
  // see if the current object matches the request
  bool objectMatchesRequest();
  // see if the current object has extra data than was requested
  bool objectHasExtraData();

  vtkUnstructuredGrid* UGrid;
  //  vtkUnstructuredGrid* Boundary_UGrid;
  bool CALC_GEOM_FLAG; // true = need to calculate continuum geometry; false = geom is up to date
  //  bool CALC_BOUNDARY_GEOM_FLAG; // true = need to calculate boundary geometry; false = boundary
  //  geom is up to date bool HAVE_BOUNDARY_GEOM_FLAG; // true = we have boundary geometry; false =
  //  geom has not been read yet

  bool READ_GEOM_FLAG; // true = need continuum geom from disk
                       //  bool READ_BOUNDARY_GEOM_FLAG; // true = need boundary geom from disk
  bool IAM_INITIALLIZED;
  bool I_HAVE_DATA;

  bool MeshIs3D;
  // int TimeStep;
  int precision;
  int blockDims[3];
  int totalBlockSize;
  int ActualTimeStep;
  int numBlocks;
  int myNumBlocks;
  int myNumBlockReads;
  int* myBlockIDs;
  int* proc_numBlocks;
  long* myBlockPositions;
  int NumberOfTimeSteps;
  double TimeValue;
  int TimeStepRange[2];
  bool swapEndian;

  std::vector<double> TimeSteps;
  //  int UseProjection;
  //  int ExtractBoundary;
  //  int DynamicMesh;
  //  double DynamicMeshScale;

  // Time query function. Called by ExecuteInformation().
  // Fills the TimestepValues array.
  bool GetAllTimesAndVariableNames(vtkInformationVector*);

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkNek5000Reader(const vtkNek5000Reader&) = delete; // Not implemented.
  void operator=(const vtkNek5000Reader&) = delete;   // Not implemented.

  int SpectralElementIds;
  int CleanGrid;
};

VTK_ABI_NAMESPACE_END
#endif
