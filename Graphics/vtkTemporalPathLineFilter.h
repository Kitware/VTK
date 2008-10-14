/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalPathLineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalPathLineFilter - Generate a Polydata Pointset from any Dataset.
//
// .SECTION Description
// vtkTemporalPathLineFilter takes any dataset as input, it extracts the point
// locations of all cells over time to build up a polyline trail.
// The point number (index) is used as the 'key' if the points are randomly 
// changing their respective order in the points list, then you should specify
// a scalar that represents the unique ID. This is intended to handle the output
// of a filter such as the TemporalStreamTracer.
// 
// .SECTION See Also
// vtkTemporalStreamTracer
//
// .SECTION Thanks
// John Bidiscombe of 
// CSCS - Swiss National Supercomputing Centre
// for creating and contributing this class.

#ifndef _vtkTemporalPathLineFilter_h
#define _vtkTemporalPathLineFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkPoints;
class vtkCellArray;
class vtkMergePoints;
class vtkFloatArray;

//BTX
#include "vtkSmartPointer.h" // for memory safety
#include <vtkstd/set>        // Because we want to use it
class ParticleTrail;
class vtkTemporalPathLineFilterInternals;
typedef vtkSmartPointer<ParticleTrail> TrailPointer;
//ETX

class VTK_GRAPHICS_EXPORT vtkTemporalPathLineFilter : public vtkPolyDataAlgorithm {
  public:
    // Description:
    // Standard Type-Macro
    static vtkTemporalPathLineFilter *New();
    vtkTypeRevisionMacro(vtkTemporalPathLineFilter,vtkPolyDataAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Set the number of particles to track as a ratio of the input
    // example: setting MaskPoints to 10 will track every 10th point
    vtkSetMacro(MaskPoints,int);
    vtkGetMacro(MaskPoints,int);
    
    // Description:
    // If the Particles being traced animate for a long time, the
    // trails or traces will become long and stringy. Setting
    // the MaxTraceTimeLength will limit how much of the trace
    // is displayed. Tracks longer then the Max will disappear
    // and the trace will apppear like a snake of fixed length
    // which progresses as the particle moves
    vtkSetMacro(MaxTrackLength,unsigned int);
    vtkGetMacro(MaxTrackLength,unsigned int);
    
    // Description:
    // True by default. We use the index of the point as the ID.
    // As the particle moves, the coords for each ID are updated to 
    // create a path. If the IDs are supplied as a separate scalar array
    // then disable this option and set the array using IdChannelArray
    vtkSetMacro(UsePointIndexForIds,int);
    vtkGetMacro(UsePointIndexForIds,int);
    vtkBooleanMacro(UsePointIndexForIds,int);
    
    // Description:
    // Specify the name of a scalar array which will be used to fetch
    // the index of each point. This is necessary only if the particles
    // change position (Id order) on each time step. The Id can be used
    // to identify particles at each step and hence track them properly
    vtkSetStringMacro(IdChannelArray);
    vtkGetStringMacro(IdChannelArray);

    // Description:
    // The particle trace can be coloured using either the time/Id
    // or the scalar value of the particle when it passed through the point
    // of the track. If colouring using a scalar, set the scalar 
    // array name to use.
    vtkSetStringMacro(ScalarArray);
    vtkGetStringMacro(ScalarArray);

    // Description:
    // If a particle disappears from one end of asimulation and reappears
    // on the other side, the track left will be unrepresentative.
    // Set a MaxStepDistance{x,y,z} which acts as a threshold above which
    // if a step occurs larger than the value (for the dimension), the track will
    // be dropped and restarted after the step. (ie the part before the wrap 
    // around will be dropped and the newer part kept).
    vtkSetVector3Macro(MaxStepDistance,double);
    vtkGetVector3Macro(MaxStepDistance,double);
    
    // Description:
    // When a particle 'disappears', the trail belonging to it is removed from
    // the list. When this flag is enabled, dead trails will persist
    // until the next time the list is cleared. Use carefully as it may cause
    // excessive memory consumption if left on by mistake.
    vtkSetMacro(KeepDeadTrails,int);
    vtkGetMacro(KeepDeadTrails,int);   

    // Description:
    // Flush will wipe any existing data so that traces can be restarted from
    // whatever time step is next supplied.
    void Flush();

    // Description:
    // Set a second input which is a selection. Particles with the same
    // Id in the selection as the primary input will be chosen for pathlines
    // Note that you must have the same IdChannelArray in the selection as the input
    void SetSelectionConnection(vtkAlgorithmOutput *algOutput);

    // Description:
    // Set a second input which is a selection. Particles with the same
    // Id in the selection as the primary input will be chosen for pathlines
    // Note that you must have the same IdChannelArray in the selection as the input
    void SetSelection(vtkDataSet *input);

  protected:
     vtkTemporalPathLineFilter();
    ~vtkTemporalPathLineFilter();

    //
    // Make sure the pipeline knows what type we expect as input
    //
    virtual int FillInputPortInformation(int port, vtkInformation* info);

    // Description:
    // The necessary parts of the standard pipeline update mechanism
    virtual int RequestInformation (vtkInformation *,
                                    vtkInformationVector **,
                                    vtkInformationVector *);
    //
    virtual int RequestData(vtkInformation *request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector);

//BTX
    TrailPointer GetTrail(vtkIdType i);
    void IncrementTrail(TrailPointer trail, vtkDataSet *input, 
      vtkDataArray *inscalars, vtkIdType i);
//ETX
    // internal data variables
    int           NumberOfTimeSteps;
    int           MaskPoints;
    unsigned int  MaxTrackLength;
    unsigned int  LastTrackLength;
    int           UsePointIndexForIds;
    int           FirstTime;
    char         *IdChannelArray;
    char         *ScalarArray;
    double        MaxStepDistance[3];
    double        LatestTime;
    int           KeepDeadTrails;
    int           UsingSelection;
    //
//BTX
    vtkSmartPointer<vtkPoints>                          ParticleCoordinates;
    vtkSmartPointer<vtkCellArray>                       ParticlePolyLines;
    vtkSmartPointer<vtkFloatArray>                      PointOpacity;
    vtkSmartPointer<vtkFloatArray>                      PointId;
    vtkSmartPointer<vtkFloatArray>                      PointScalars;
    vtkSmartPointer<vtkTemporalPathLineFilterInternals> Internals;
    vtkstd::set<vtkIdType>                              SelectionIds;
//ETX
    //
  private:
    vtkTemporalPathLineFilter(const vtkTemporalPathLineFilter&);  // Not implemented.
    void operator=(const vtkTemporalPathLineFilter&);  // Not implemented.
};

#endif
