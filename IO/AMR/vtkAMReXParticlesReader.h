/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMReXParticlesReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkAMReXParticlesReader
 * @brief reader for AMReX plotfiles particle data.
 *
 * vtkAMReXParticlesReader readers particle data from AMReX plotfiles. The
 * reader is based on the  `ParticleContainer::Restart` and
 * `amrex_binary_particles_to_vtp` files in the
 * [AMReX code](https://amrex-codes.github.io/).
 *
 * The reader reads all levels in as blocks in output multiblock dataset
 * distributed datasets at each level between ranks in a contiguous fashion.
 *
 * To use the reader, one must set the `PlotFileName` and `ParticleType` which
 * identifies the type particles from the PlotFileName to read.
 *
 * The reader provides ability to select point data arrays to be made available
 * in the output. Note that due to the nature of the file structure, all
 * variables are still read in and hence deselecting arrays does not reduce I/O
 * calls or initial memory requirements.
 */

#ifndef vtkAMReXParticlesReader_h
#define vtkAMReXParticlesReader_h

#include "vtkIOAMRModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h" // for vtkNew

#include <string> // for std::string.

class vtkDataArraySelection;
class vtkMultiPieceDataSet;
class vtkMultiProcessController;

class VTKIOAMR_EXPORT vtkAMReXParticlesReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMReXParticlesReader* New();
  vtkTypeMacro(vtkAMReXParticlesReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the AMReX plotfile. Note this is a directory on the filesystem and
   * not the file.
   */
  void SetPlotFileName(const char* fname);
  const char* GetPlotFileName() const;
  //@}

  //@{
  /**
   * Get/Set the particle type to read. By default, this is set to 'particles'.
   */
  void SetParticleType(const std::string& str);
  const std::string& GetParticleType() const { return this->ParticleType; }
  //@}

  /**
   * Get vtkDataArraySelection instance to select point arrays to read. Due to
   * the nature of the AMReX particles files, all point data is read in from the
   * disk, despite certain arrays unselected. The unselected arrays will be
   * discarded from the generated output dataset.
   */
  vtkDataArraySelection* GetPointDataArraySelection() const;

  /**
   * Returns 1 is fname refers to a plotfile that the reader can read.
   */
  static int CanReadFile(const char* fname, const char* particlesType = nullptr);

  //@{
  /**
   * Get/Set the controller to use. By default, the global
   * vtkMultiProcessController will be used.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkAMReXParticlesReader();
  ~vtkAMReXParticlesReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;
  std::string PlotFileName;
  bool GenerateGlobalIds;

private:
  vtkAMReXParticlesReader(const vtkAMReXParticlesReader&) = delete;
  void operator=(const vtkAMReXParticlesReader&) = delete;

  /**
   * Reads the header and fills up this->Header data-structure.
   */
  bool ReadMetaData();

  /**
   * Reads a level. Blocks in the level are distributed among pieces in a
   * contiguous fashion.
   */
  bool ReadLevel(const int level, vtkMultiPieceDataSet* pdataset, const int piece_idx,
    const int num_pieces) const;

  vtkTimeStamp PlotFileNameMTime;
  vtkTimeStamp MetaDataMTime;
  std::string ParticleType;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;

  class AMReXParticleHeader;
  AMReXParticleHeader* Header;
  friend class AMReXParticleHeader;
};

#endif
