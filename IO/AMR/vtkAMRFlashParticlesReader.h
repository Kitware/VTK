/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRFlashParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRFlashParticlesReader.cxx -- Reads Flash particles dataset
//
// .SECTION Description
//  A concrete instance of vtkAMRBaseParticlesReader that implements
//  functionality for reading flash particle datasets.
#ifndef VTKAMRFLASHPARTICLESREADER_H_
#define VTKAMRFLASHPARTICLESREADER_H_

#include "vtkIOAMRModule.h" // For export macro
#include "vtkAMRBaseParticlesReader.h"

class vtkIndent;
class vtkPolyData;
class vtkPointData;
class vtkIdList;
class vtkFlashReaderInternal;

class VTKIOAMR_EXPORT vtkAMRFlashParticlesReader :
  public vtkAMRBaseParticlesReader
{
public:
  static vtkAMRFlashParticlesReader* New();
  vtkTypeMacro( vtkAMRFlashParticlesReader, vtkAMRBaseParticlesReader );
  void PrintSelf(ostream &os, vtkIndent indent );

  // Description:
  // See vtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
  int GetTotalNumberOfParticles();

protected:
  vtkAMRFlashParticlesReader();
  virtual ~vtkAMRFlashParticlesReader();

  // Description:
  // See vtkAMRBaseParticlesReader::ReadMetaData
  void ReadMetaData();

  // Description:
  // See vtkAMRBaseParticlesReader::SetupParticlesDataSelections
  void SetupParticleDataSelections();

  // Description:
  // See vtkAMRBaseParticlesReader::ReadParticles
  vtkPolyData* ReadParticles( const int blkidx );

  // Description:
  // Reads the particlles of the given block from the given file.
  vtkPolyData* GetParticles( const char* file, const int blkidx );

  vtkFlashReaderInternal *Internal;

private:
  vtkAMRFlashParticlesReader( const vtkAMRFlashParticlesReader& ); // Not implemented
  void operator=(const vtkAMRFlashParticlesReader& ); // Not implemented
};

#endif /* VTKAMRFLASHPARTICLESREADER_H_ */
