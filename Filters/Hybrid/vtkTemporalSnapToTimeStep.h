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
/**
 * @class   vtkTemporalSnapToTimeStep
 * @brief   modify the time range/steps of temporal data
 *
 * vtkTemporalSnapToTimeStep  modify the time range or time steps of
 * the data without changing the data itself. The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 *
 * @par Thanks:
 * John Bidiscombe of CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
*/

#ifndef vtkTemporalSnapToTimeStep_h
#define vtkTemporalSnapToTimeStep_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkAlgorithm.h"

#include <vector> // used because I am a bad boy. So there.

class VTKFILTERSHYBRID_EXPORT vtkTemporalSnapToTimeStep : public vtkAlgorithm
{
public:
  static vtkTemporalSnapToTimeStep *New();
  vtkTypeMacro(vtkTemporalSnapToTimeStep, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum {
    VTK_SNAP_NEAREST=0,
    VTK_SNAP_NEXTBELOW_OR_EQUAL,
    VTK_SNAP_NEXTABOVE_OR_EQUAL
  };

  vtkSetMacro(SnapMode,int);
  vtkGetMacro(SnapMode,int);
  void SetSnapModeToNearest()          { this->SetSnapMode(VTK_SNAP_NEAREST); }
  void SetSnapModeToNextBelowOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTBELOW_OR_EQUAL); }
  void SetSnapModeToNextAboveOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTABOVE_OR_EQUAL); }

protected:
  vtkTemporalSnapToTimeStep();
  ~vtkTemporalSnapToTimeStep();

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info);

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

    std::vector<double>  InputTimeValues;
    int HasDiscrete;
    int SnapMode;

private:
  vtkTemporalSnapToTimeStep(const vtkTemporalSnapToTimeStep&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalSnapToTimeStep&) VTK_DELETE_FUNCTION;
};



#endif



