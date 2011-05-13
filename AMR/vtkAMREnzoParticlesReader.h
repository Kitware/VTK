/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMREnzoParticlesReader.h -- Reads AMR Enzo Particle datasets
//
// .SECTION Description
//  A concrete instance of the vtkAMRBaseParticlesReader which provides
//  functionality for loading ENZO AMR particle datasets from ENZO.
//
// .SECTION See Also
//  vtkAMRBaseParticlesReader

#ifndef VTKAMRENZOPARTICLESREADER_H_
#define VTKAMRENZOPARTICLESREADER_H_

#include "vtkAMRBaseParticlesReader.h"


class vtkPolyData;
class vtkEnzoReaderInternal;

class VTK_AMR_EXPORT vtkAMREnzoParticlesReader :
  public vtkAMRBaseParticlesReader
{
  public:
    static vtkAMREnzoParticlesReader* New();
    vtkTypeMacro( vtkAMREnzoParticlesReader, vtkAMRBaseParticlesReader );
    void PrintSelf( std::ostream &os, vtkIndent indent );

  protected:
    vtkAMREnzoParticlesReader();
    virtual ~vtkAMREnzoParticlesReader();

    // Description:
    // Read the particles from the given particles file for the block
    // corresponding to the given block index.
    vtkPolyData* GetParticles( const char* file, const int blockIdx );

    // Description:
    // See vtkAMRBaseParticlesReader::ReadMetaData()
    void ReadMetaData();

    // Description:
    // See vtkAMRBaseParticlesReader::SetupParticleDataSelections
    void SetupParticleDataSelections();


    // Description:
    // Reads the particles.
    vtkPolyData* ReadParticles( const int blkidx );

    vtkEnzoReaderInternal *Internal;

  private:
    vtkAMREnzoParticlesReader( const vtkAMREnzoParticlesReader& ); // Not implemented
    void operator=( const vtkAMREnzoParticlesReader& ); // Not implemented
};

#endif /* VTKAMRENZOPARTICLESREADER_H_ */
