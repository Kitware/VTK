/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalSnapToTimeStep.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalSnapToTimeStep - modify the time range/steps of temporal data
// .SECTION Description
// vtkTemporalSnapToTimeStep  modify the time range or time steps of
// the data without changing the data itself. The data is not resampled
// by this filter, only the information accompanying the data is modified.

#ifndef __vtkTemporalSnapToTimeStep_h
#define __vtkTemporalSnapToTimeStep_h

#include "vtkTemporalDataSetAlgorithm.h"

//BTX
#include <vtkstd/vector>
//ETX

class VTK_HYBRID_EXPORT vtkTemporalSnapToTimeStep : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalSnapToTimeStep *New();
  vtkTypeRevisionMacro(vtkTemporalSnapToTimeStep, vtkTemporalDataSetAlgorithm);

//BTX
  enum {
    VTK_SNAP_NEAREST=0,
    VTK_SNAP_NEXTBELOW_OR_EQUAL,
    VTK_SNAP_NEXTABOVE_OR_EQUAL
  };
//ETX
  vtkSetMacro(SnapMode,int);
  vtkGetMacro(SnapMode,int);
  void SetSnapModeToNearest()          { this->SetSnapMode(VTK_SNAP_NEAREST); }
  void SetSnapModeToNextBelowOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTBELOW_OR_EQUAL); }
  void SetSnapModeToNextAboveOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTABOVE_OR_EQUAL); }

protected:
  vtkTemporalSnapToTimeStep();
  ~vtkTemporalSnapToTimeStep();

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

//BTX
    vtkstd::vector<double>  InputTimeValues;
    int HasDiscrete;
    int SnapMode;
//ETX

private:
  vtkTemporalSnapToTimeStep(const vtkTemporalSnapToTimeStep&);  // Not implemented.
  void operator=(const vtkTemporalSnapToTimeStep&);  // Not implemented.
};



#endif



