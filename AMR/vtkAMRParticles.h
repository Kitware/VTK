/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRParticles.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRParticles.h -- Object to store AMR particles.
//
// .SECTION Description
// A concrete instance of vtkObject that provides functionality
// for storing and accessing particles.
//
// .SECTION See Also
// vtkAMRParticleType.h

#ifndef VTKAMRPARTICLES_H_
#define VTKAMRPARTICLES_H_

#include "vtkObject.h"

class vtkIndent;
class vtkPointData;
class vtkPoints;

class VTK_AMR_EXPORT vtkAMRParticles : public vtkObject
{
  public:
    static vtkAMRParticles* New();
    vtkTypeMacro( vtkAMRParticles, vtkObject );
    void PrintSelf( std::ostream &os, vtkIndent indent );

    // Description:
    // Get & Set of the particle type attribute.
    vtkSetMacro( ParticleType, int );
    vtkGetMacro( ParticleType, int );

    // Description:
    // Sets the total number of particles.
    void SetNumberOfParticles( vtkIdType N );

    // Description:
    // Sets the particle corresponding to index idx.
    void SetParticle(const vtkIdType idx, double x[3] );
    void SetParticle(const vtkIdType idx,
        const double x, const double y, const double z );

    // Description:
    // Gets the particles at index idx.
    void GetParticle(const vtkIdType idx, double x[3] );
    void GetParticle(const vtkIdType idx, double &x, double &y, double &z );

    // Description:
    // Returns the total number of particles.
    vtkIdType GetNumberOfParticles();

    // Description:
    // Returns the data associated with this instance of vtkAMRParticles.
    vtkPointData* GetParticleData();

  protected:
    vtkAMRParticles();
    virtual ~vtkAMRParticles();

    int ParticleType;
    vtkPoints *Points;
    vtkPointData *ParticleData;
  private:
    vtkAMRParticles( const vtkAMRParticles& ); // Not implemented
    void operator=( const vtkAMRParticles& ); // Not implemented
};

#endif /* VTKAMRPARTICLES_H_ */
