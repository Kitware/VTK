/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMotionFXCFGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMotionFXCFGReader_h
#define vtkMotionFXCFGReader_h

#include "vtkIOMotionFXModule.h" // for export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <string> // for std::string

/**
 * @class vtkMotionFXCFGReader
 * @brief reader for MotionFX motion definitions cfg files.
 *
 * MotionFX files comprise of `motion`s for a collection of STL files. The
 * motions define the transformations to apply to STL geometry to emulate
 * motion like translation, rotation, planetary motion, etc.
 *
 * This reader reads such a CFG file and produces a temporal output for the time
 * range defined in the file. The resolution of time can be controlled using the
 * `SetTimeResolution` method. The output is a multiblock dataset with blocks
 * for each of bodies, identified by an STL file, in the cfg file.
 *
 * The reader uses PEGTL (https://github.com/taocpp/PEGTL)
 * to define and parse the grammar for the CFG file.
 */
class VTKIOMOTIONFX_EXPORT vtkMotionFXCFGReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMotionFXCFGReader* New();
  vtkTypeMacro(vtkMotionFXCFGReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the filename.
   */
  void SetFileName(const char* fname);
  const char* GetFileName() const
  {
    return this->FileName.empty() ? nullptr : this->FileName.c_str();
  }
  //@}

  //@{
  /**
   * Get/Set the time resolution for timesteps produced by the reader.
   */
  vtkSetClampMacro(TimeResolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(TimeResolution, int);
  //@}

protected:
  vtkMotionFXCFGReader();
  ~vtkMotionFXCFGReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Reads meta-data. Returns false if file not readable.
   */
  bool ReadMetaData();

  std::string FileName;
  int TimeResolution;
  vtkTimeStamp FileNameMTime;
  vtkTimeStamp MetaDataMTime;

private:
  vtkMotionFXCFGReader(const vtkMotionFXCFGReader&) = delete;
  void operator=(const vtkMotionFXCFGReader&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
