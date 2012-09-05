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

#include "vtkIOAMRModule.h" // For export macro
#include "vtkAMRBaseParticlesReader.h"


class vtkPolyData;
class vtkDataArray;
class vtkIntArray;
class vtkEnzoReaderInternal;

class VTKIOAMR_EXPORT vtkAMREnzoParticlesReader :
  public vtkAMRBaseParticlesReader
{
public:
  static vtkAMREnzoParticlesReader* New();
  vtkTypeMacro( vtkAMREnzoParticlesReader, vtkAMRBaseParticlesReader );
  void PrintSelf(ostream &os, vtkIndent indent );

  // Description:
  // Returns the requested particle type.
  vtkSetMacro( ParticleType, int );
  vtkGetMacro( ParticleType, int );

  // Description:
  // See vtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
  int GetTotalNumberOfParticles();

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
  // Filter's by particle type, iff particle_type is included in
  // the given file.
  bool CheckParticleType( const int pIdx, vtkIntArray *ptypes );

  // Description:
  // Returns the ParticlesType Array
  vtkDataArray *GetParticlesTypeArray( const int blockIdx );

  // Description:
  // Reads the particles.
  vtkPolyData* ReadParticles( const int blkidx );

  int ParticleType;

  vtkEnzoReaderInternal *Internal;

private:
  vtkAMREnzoParticlesReader( const vtkAMREnzoParticlesReader& ); // Not implemented
  void operator=( const vtkAMREnzoParticlesReader& ); // Not implemented
};

#endif /* VTKAMRENZOPARTICLESREADER_H_ */
