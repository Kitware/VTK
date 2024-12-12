// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalPathLineFilter
 * @brief   Generate a Polydata Pointset from any Dataset.
 *
 *
 * vtkTemporalPathLineFilter takes any dataset as input, it extracts the point
 * locations of all cells over time to build up a polyline trail.
 * The point number (index) is used as the 'key' if the points are randomly
 * changing their respective order in the points list, then you should specify
 * a scalar that represents the unique ID. This is intended to handle the output
 * of a filter such as the vtkParticleTracer.
 *
 * @sa
 * vtkParticleTracer
 *
 * @par Thanks:
 * John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 */

#ifndef vtkTemporalPathLineFilter_h
#define vtkTemporalPathLineFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkTemporalAlgorithm.h" // For temporal algorithm

#ifndef __VTK_WRAP__
#define vtkPolyDataAlgorithm vtkTemporalAlgorithm<vtkPolyDataAlgorithm>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkCellArray;
class vtkMergePoints;
class vtkFloatArray;

VTK_ABI_NAMESPACE_END
#include "vtkSmartPointer.h" // for memory safety
#include <set>               // Because we want to use it
VTK_ABI_NAMESPACE_BEGIN
class ParticleTrail;
class vtkTemporalPathLineFilterInternals;
typedef vtkSmartPointer<ParticleTrail> TrailPointer;

class VTKFILTERSGENERAL_EXPORT vtkTemporalPathLineFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard Type-Macro
   */
  vtkTypeMacro(vtkTemporalPathLineFilter, vtkPolyDataAlgorithm);
#ifndef __VTK_WRAP__
#undef vtkPassInputTypeAlgorithm
#endif
  static vtkTemporalPathLineFilter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML)
  vtkCreateWrappedTemporalAlgorithmInterface();
#endif

  ///@{
  /**
   * Set the number of particles to track as a ratio of the input
   * example: setting MaskPoints to 10 will track every 10th point
   */
  vtkSetMacro(MaskPoints, int);
  vtkGetMacro(MaskPoints, int);
  ///@}

  ///@{
  /**
   * If the Particles being traced animate for a long time, the
   * trails or traces will become long and stringy. Setting
   * the MaxTraceTimeLength will limit how much of the trace
   * is displayed. Tracks longer then the Max will disappear
   * and the trace will appear like a snake of fixed length
   * which progresses as the particle moves
   */
  vtkSetMacro(MaxTrackLength, unsigned int);
  vtkGetMacro(MaxTrackLength, unsigned int);
  ///@}

  ///@{
  /**
   * Specify the name of a scalar array which will be used to fetch
   * the index of each point. This is necessary only if the particles
   * change position (Id order) on each time step. The Id can be used
   * to identify particles at each step and hence track them properly.
   * If this array is nullptr, the global point ids are used.  If an Id
   * array cannot otherwise be found, the point index is used as the ID.
   */
  vtkSetStringMacro(IdChannelArray);
  vtkGetStringMacro(IdChannelArray);
  ///@}

  ///@{
  /**
   * If a particle disappears from one end of a simulation and reappears
   * on the other side, the track left will be unrepresentative.
   * Set a MaxStepDistance{x,y,z} which acts as a threshold above which
   * if a step occurs larger than the value (for the dimension), the track will
   * be dropped and restarted after the step. (ie the part before the wrap
   * around will be dropped and the newer part kept).
   */
  vtkSetVector3Macro(MaxStepDistance, double);
  vtkGetVector3Macro(MaxStepDistance, double);
  ///@}

  ///@{
  /**
   * When a particle 'disappears', the trail belonging to it is removed from
   * the list. When this flag is enabled, dead trails will persist
   * until the next time the list is cleared. Use carefully as it may cause
   * excessive memory consumption if left on by mistake.
   */
  vtkSetMacro(KeepDeadTrails, bool);
  vtkGetMacro(KeepDeadTrails, bool);
  ///@}

  /**
   * Flush will wipe any existing data so that traces can be restarted from
   * whatever time step is next supplied.
   */
  void Flush();

  /**
   * Set a second input which is a selection. Particles with the same
   * Id in the selection as the primary input will be chosen for pathlines
   * Note that you must have the same IdChannelArray in the selection as the input
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Set a second input which is a selection. Particles with the same
   * Id in the selection as the primary input will be chosen for pathlines
   * Note that you must have the same IdChannelArray in the selection as the input
   */
  void SetSelectionData(vtkDataSet* input);

protected:
  vtkTemporalPathLineFilter();
  ~vtkTemporalPathLineFilter() override;

  //
  // Make sure the pipeline knows what type we expect as input
  //
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int Initialize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int Execute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int Finalize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  void IncrementTrail(TrailPointer trail, vtkDataSet* input, vtkIdType i);

  TrailPointer GetTrail(vtkIdType i);

  // void Initialize(vtkDataSet* input, vtkDataSet* selection,
  //  vtkPolyData* pathLines, vtkPolyData* particles);

  // internal data variables
  int NumberOfTimeSteps = 0;
  int MaskPoints = 200;
  unsigned int MaxTrackLength = 10;
  unsigned int LastTrackLength = 10;
  char* IdChannelArray = nullptr;
  double MaxStepDistance[3] = { 1, 1, 1 };
  double LatestTime;
  bool KeepDeadTrails = false;
  bool BackwardTime = false;
  //

  vtkSmartPointer<vtkCellArray> PolyLines;
  vtkSmartPointer<vtkCellArray> Vertices;
  vtkSmartPointer<vtkPoints> LineCoordinates;
  vtkSmartPointer<vtkPoints> VertexCoordinates;
  vtkSmartPointer<vtkFloatArray> TrailId;
  vtkSmartPointer<vtkTemporalPathLineFilterInternals> Internals;

  //
private:
  void AccumulateTrails(vtkDataSet* input, vtkDataSet* selection);
  void PostExecute(vtkDataSet* input, vtkPolyData* pathLines, vtkPolyData* particles);
  void InitializeExecute(vtkDataSet* input, vtkPolyData* pathLines);

  vtkTemporalPathLineFilter(const vtkTemporalPathLineFilter&) = delete;
  void operator=(const vtkTemporalPathLineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
