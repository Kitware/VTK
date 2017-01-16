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
/**
 * @class   vtkAMREnzoParticlesReader
 *
 *
 *  A concrete instance of the vtkAMRBaseParticlesReader which provides
 *  functionality for loading ENZO AMR particle datasets from ENZO.
 *
 * @sa
 *  vtkAMRBaseParticlesReader
*/

#ifndef vtkAMREnzoParticlesReader_h
#define vtkAMREnzoParticlesReader_h

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
  void PrintSelf(ostream &os, vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Returns the requested particle type.
   */
  vtkSetMacro( ParticleType, int );
  vtkGetMacro( ParticleType, int );
  //@}

  /**
   * See vtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
   */
  int GetTotalNumberOfParticles() VTK_OVERRIDE;

protected:
  vtkAMREnzoParticlesReader();
  ~vtkAMREnzoParticlesReader() VTK_OVERRIDE;

  /**
   * Read the particles from the given particles file for the block
   * corresponding to the given block index.
   */
  vtkPolyData* GetParticles( const char* file, const int blockIdx );

  /**
   * See vtkAMRBaseParticlesReader::ReadMetaData()
   */
  void ReadMetaData() VTK_OVERRIDE;

  /**
   * See vtkAMRBaseParticlesReader::SetupParticleDataSelections
   */
  void SetupParticleDataSelections() VTK_OVERRIDE;

  /**
   * Filter's by particle type, iff particle_type is included in
   * the given file.
   */
  bool CheckParticleType( const int pIdx, vtkIntArray *ptypes );

  /**
   * Returns the ParticlesType Array
   */
  vtkDataArray *GetParticlesTypeArray( const int blockIdx );

  /**
   * Reads the particles.
   */
  vtkPolyData* ReadParticles( const int blkidx ) VTK_OVERRIDE;

  int ParticleType;

  vtkEnzoReaderInternal *Internal;

private:
  vtkAMREnzoParticlesReader( const vtkAMREnzoParticlesReader& ) VTK_DELETE_FUNCTION;
  void operator=( const vtkAMREnzoParticlesReader& ) VTK_DELETE_FUNCTION;
};

#endif /* vtkAMREnzoParticlesReader_h */
