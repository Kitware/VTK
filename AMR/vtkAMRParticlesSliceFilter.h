/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRParticlesSliceFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRParticlesSliceFilter.h -- Particle "slice" filter
//
// .SECTION Description
//  A concrete instance of vtkMultiBlockDataSetAlgorithm which implements
//  functionality for AMR volumetric particle datasets. Given a plane offset,
//  a normal and distance from the plane delta, the filter extracts all
//  particles within delta from the slice plane.
//
// .SECTION See Also
//  vtkAMRBaseParticlesReader vtkARMSliceFilter

#ifndef VTKAMRPARTICLESSLICEFILTER_H_
#define VTKAMRPARTICLESSLICEFILTER_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;

class VTK_AMR_EXPORT vtkAMRParticlesSliceFilter :
    public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRParticlesSliceFilter* New();
    vtkTypeMacro( vtkAMRParticlesSliceFilter, vtkMultiBlockDataSetAlgorithm );
    void PrintSelf( std::ostream &os, vtkIndent indent );

    // Description:
    // Set/Get the Axis normal. There are only 3 acceptable values
    // 1-(X-Normal); 2-(Y-Normal); 3-(Z-Normal)
    vtkSetMacro(Normal,int);
    vtkGetMacro(Normal,int);

    // Description:
    // Inline Gettters & Setters
    vtkSetMacro(OffSetFromOrigin,double);
    vtkGetMacro(OffSetFromOrigin,double);

    // Description:
    // Set/Get DX -- the distance threshold from the slice that includes particles.
    vtkSetMacro(DX,double);
    vtkGetMacro(DX,double);

    // Standard Pipeline methods
    virtual int RequestData(
        vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

  protected:
    vtkAMRParticlesSliceFilter();
    ~vtkAMRParticlesSliceFilter();

    double OffSetFromOrigin;
    double origin[3];
    double DX;
    int Normal;
    int Frequency;
  private:
    vtkAMRParticlesSliceFilter( const vtkAMRParticlesSliceFilter& ); // Not implemented
    void operator=( const vtkAMRParticlesSliceFilter& ); // Not implemented
};

#endif /* VTKAMRPARTICLESSLICEFILTER_H_ */
