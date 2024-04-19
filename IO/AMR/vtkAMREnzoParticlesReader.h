// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkAMRBaseParticlesReader.h"
#include "vtkIOAMRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkDataArray;
class vtkIntArray;
class vtkEnzoReaderInternal;

class VTKIOAMR_EXPORT vtkAMREnzoParticlesReader : public vtkAMRBaseParticlesReader
{
public:
  static vtkAMREnzoParticlesReader* New();
  vtkTypeMacro(vtkAMREnzoParticlesReader, vtkAMRBaseParticlesReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns the requested particle type.
   */
  vtkSetMacro(ParticleType, int);
  vtkGetMacro(ParticleType, int);
  ///@}

  /**
   * See vtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
   */
  int GetTotalNumberOfParticles() override;

protected:
  vtkAMREnzoParticlesReader();
  ~vtkAMREnzoParticlesReader() override;

  /**
   * Read the particles from the given particles file for the block
   * corresponding to the given block index.
   */
  vtkPolyData* GetParticles(const char* file, int blockIdx);

  /**
   * See vtkAMRBaseParticlesReader::ReadMetaData()
   */
  void ReadMetaData() override;

  /**
   * See vtkAMRBaseParticlesReader::SetupParticleDataSelections
   */
  void SetupParticleDataSelections() override;

  /**
   * Filter's by particle type, iff particle_type is included in
   * the given file.
   */
  bool CheckParticleType(int pIdx, vtkIntArray* ptypes);

  /**
   * Returns the ParticlesType Array
   */
  vtkDataArray* GetParticlesTypeArray(int blockIdx);

  /**
   * Reads the particles.
   */
  vtkPolyData* ReadParticles(int blkidx) override;

  int ParticleType;

  vtkEnzoReaderInternal* Internal;

private:
  vtkAMREnzoParticlesReader(const vtkAMREnzoParticlesReader&) = delete;
  void operator=(const vtkAMREnzoParticlesReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkAMREnzoParticlesReader_h */
