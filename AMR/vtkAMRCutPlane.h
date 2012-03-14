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

#include <vector> // For STL vector

class vtkMultiBlockDataSet;
class vtkOverlappingAMR;
class vtkMultiProcessController;
class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkPlane;
class vtkPointLocator;
class vtkContourValues;
class vtkUniformGrid;
class vtkCell;
class vtkPoints;
class vtkLocator;
class vtkCellArray;

class VTK_AMR_EXPORT vtkAMRCutPlane : public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRCutPlane *New();
    vtkTypeMacro(vtkAMRCutPlane, vtkMultiBlockDataSetAlgorithm);
    void PrintSelf(ostream &oss, vtkIndent indent );

    // Description:
    // Sets the center
    vtkSetVector3Macro(Center, double);

    // Description:
    // Sets the normal
    vtkSetVector3Macro(Normal, double);

    // Description:
    // Sets the level of resolution
    vtkSetMacro(LevelOfResolution, int);
    vtkGetMacro(LevelOfResolution, int);

    // Description:
    //
    vtkSetMacro(UseNativeCutter, int);
    vtkGetMacro(UseNativeCutter, int);
    vtkBooleanMacro(UseNativeCutter,int);

    // Description:
    // Set/Get a multiprocess controller for paralle processing.
    // By default this parameter is set to NULL by the constructor.
    vtkSetMacro( Controller, vtkMultiProcessController* );
    vtkGetMacro( Controller, vtkMultiProcessController* );

    // Standard pipeline routines

    virtual int RequestData(
         vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);


    // Description:
    // Gets the metadata from upstream module and determines which blocks
    // should be loaded by this instance.
    virtual int RequestInformation(
        vtkInformation *rqst,
        vtkInformationVector **inputVector,
        vtkInformationVector *outputVector );

    // Description:
    // Performs upstream requests to the reader
    virtual int RequestUpdateExtent(
        vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  protected:
    vtkAMRCutPlane();
    virtual ~vtkAMRCutPlane();

    // Description:
    // Returns the cut-plane defined by a vtkCutPlane instance based on the
    // user-supplied center and normal.
    vtkPlane* GetCutPlane( vtkOverlappingAMR *metadata );

    // Description:
    // Extracts cell
    void ExtractCellFromGrid(
        vtkUniformGrid *grid,
        vtkCell* cell, vtkLocator *loc,
        vtkPoints *pts, vtkCellArray *cells );

    // Description:
    // Given a cut-plane, p, and the metadata, m, this method computes which
    // blocks need to be loaded. The corresponding block IDs are stored in
    // the internal STL vector, blocksToLoad, which is then propagated upstream
    // in the RequestUpdateExtent.
    void ComputeAMRBlocksToLoad( vtkPlane* p, vtkOverlappingAMR* m);

    // Descriription:
    // Initializes the cut-plane center given the min/max bounds.
    void InitializeCenter( double min[3], double max[3] );

    // Description:
    // Determines if a plane intersects with an AMR box
    bool PlaneIntersectsAMRBox( double bounds[6] );
    bool PlaneIntersectsAMRBox( double plane[4], double bounds[6] );

    // Description:
    // Determines if a plane intersects with a grid cell
    bool PlaneIntersectsCell( vtkCell *cell );

    // Description:
    // A utility function that checks if the input AMR data is 2-D.
    bool IsAMRData2D( vtkOverlappingAMR *input );

    // Description:
    // Applies cutting to an AMR block
    void CutAMRBlock( vtkUniformGrid *grid, vtkMultiBlockDataSet *dataSet );

    int    LevelOfResolution;
    double Center[3];
    double Normal[3];
    bool initialRequest;
    bool UseNativeCutter;
    vtkMultiProcessController *Controller;
    vtkPlane *Plane;
    vtkContourValues *contourValues;

// BTX
    std::vector<int> blocksToLoad;
// ETX

  private:
    vtkAMRCutPlane(const vtkAMRCutPlane& ); // Not implemented
    void operator=(const vtkAMRCutPlane& ); // Not implemented
};

#endif /* VTKAMRCUTPLANE_H_ */
