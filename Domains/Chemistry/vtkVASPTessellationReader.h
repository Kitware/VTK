/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVASPTessellationReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkVASPTessellationReader
 * @brief   Read NPT_Z_TESSELLATE.out files.
 *
 *
 * Read NPT_Z_TESSELLATE.out files from VASP.
*/

#ifndef vtkVASPTessellationReader_h
#define vtkVASPTessellationReader_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

namespace vtksys {
class RegularExpression;
}

class vtkUnstructuredGrid;

class VTKDOMAINSCHEMISTRY_EXPORT vtkVASPTessellationReader
    : public vtkMoleculeAlgorithm
{
public:
  static vtkVASPTessellationReader* New();
  vtkTypeMacro(vtkVASPTessellationReader, vtkMoleculeAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The name of the file to read.
   */
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  //@}

protected:
  vtkVASPTessellationReader();
  ~vtkVASPTessellationReader() VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inInfoVecs,
                          vtkInformationVector *outInfoVec) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inInfoVecs,
                                 vtkInformationVector *outInfoVec) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  /**
   * Advance @a in to the start of the data for the next timestep. Parses the
   * "time = X" line, sets @a time to the timestamp, and returns true on
   * success. Returning false means either EOF was reached, or the timestamp
   * line could not be parsed.
   */
  bool NextTimeStep(std::istream &in, double &time);

  /**
   * Called by RequestData to determine which timestep to read. If both
   * UPDATE_TIME_STEP and TIME_STEPS are defined, return the index of the
   * timestep in TIME_STEPS closest to UPDATE_TIME_STEP. If either is undefined,
   * return 0.
   */
  size_t SelectTimeStepIndex(vtkInformation *info);

  bool ReadTimeStep(std::istream &in, vtkMolecule *molecule,
                    vtkUnstructuredGrid *voronoi);

  char *FileName;

  vtksys::RegularExpression *TimeParser;
  vtksys::RegularExpression *LatticeParser;
  vtksys::RegularExpression *AtomCountParser;
  vtksys::RegularExpression *AtomParser;
  vtksys::RegularExpression *ParenExtract;

private:
  vtkVASPTessellationReader(const vtkVASPTessellationReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVASPTessellationReader&) VTK_DELETE_FUNCTION;
};

#endif // vtkVASPTessellationReader_h
