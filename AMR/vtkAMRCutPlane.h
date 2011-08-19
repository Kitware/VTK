/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRCutPlane.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRCutPlane.h -- Cuts an AMR dataset
//
// .SECTION Description
//  TODO: Enter documentation here!

#ifndef VTKAMRCUTPLANE_H_
#define VTKAMRCUTPLANE_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

class VTK_AMR_EXPORT vtkAMRCutPlane : public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRCutPlane *New();
    vtkTypeMacro(vtkAMRCutPlane, vtkMultiBlockDataSetAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

    // Description:
    // Sets the center
    vtkSetVector3Macro(Center, double);

    // Description:
    // Sets the normal
    vtkSetVector3Macro(Normal, double);

    // Description:
    // Sets the level of resolution
    vtkSetMacro(LevelOfResolution, int);

    // Standard pipeline routines

    // Description:
    // Gets the metadata from upstream module and determines which blocks
    // should be loaded by this instance.
    virtual int RequestInformation(
        vtkInformation *rqst,
        vtkInformationVector **inputVector,
        vtkInformationVector *outputVector );

    virtual int RequestData(
         vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    // Description:
    // Performs upstream requests to the reader
    virtual int RequestUpdateExtent(
        vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  protected:
    vtkAMRCutPlane();
    virtual ~vtkAMRCutPlane();

    int    LevelOfResolution;
    double Center[3];
    double Normal[3];

  private:
    vtkAMRCutPlane(const vtkAMRCutPlane& ); // Not implemented
    void operator=(const vtkAMRCutPlane& ); // Not implemented
};

#endif /* VTKAMRCUTPLANE_H_ */
