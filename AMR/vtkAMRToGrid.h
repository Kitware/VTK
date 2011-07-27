/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRToUniformGrid.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRToGrid.h -- Resamples AMR data to a uniform grid.
//
// .SECTION Description
//  This filter is a concrete instance of vtkMultiBlockDataSetAlgorithm and
//  provides functionality for extracting portion of the AMR dataset, specified
//  by a bounding box, in a uniform grid of the desired level of resolution.
//  The resulting uniform grid is stored in a vtkMultiBlockDataSet where the
//  number of blocks correspond to the number of processors utilized for the
//  operation.

#ifndef VTKAMRTOUNIFORMGRID_H_
#define VTKAMRTOUNIFORMGRID_H_

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSetGet.h"

class vtkInformation;
class vtkInformationVector;
class vtkUniformGrid;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkIndent;

class VTK_AMR_EXPORT vtkAMRToGrid : public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRToGrid *New();
    vtkTypeMacro(vtkAMRToGrid,vtkMultiBlockDataSetAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent);

    // Description:
    // Set & Get macro for the level of resolution.
    vtkSetMacro(LevelOfResolution,int);
    vtkGetMacro(LevelOfResolution,int);

    // Description:
    // Set & Get macro for the TransferToNodes flag
    vtkSetMacro(TransferToNodes,int);
    vtkGetMacro(TransferToNodes,int);

    // Description:
    // Setter for the min/max bounds
    vtkSetVector3Macro(Min,double);
    vtkSetVector3Macro(Max,double);

    // Description:
    // Set & Get macro for the multi-process controller
    vtkSetMacro(Controller, vtkMultiProcessController*);
    vtkGetMacro(Controller, vtkMultiProcessController*);

  protected:
    vtkAMRToGrid();
    virtual ~vtkAMRToGrid();


    int TransferToNodes;

    int LevelOfResolution;

    vtkMultiProcessController *Controller;

    double Min[3];
    double Max[3];

    // Standard pipeline routines
    virtual int RequestData(
         vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

  private:
    vtkAMRToGrid(const vtkAMRToGrid&); // Not implemented
    void operator=(const vtkAMRToGrid&); // Not implemented
};

#endif /* VTKAMRTOUNIFORMGRID_H_ */
