/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRBaseParticlesReader.h -- Base class for all AMR particle readers
//
// .SECTION Description
//  TODO: Enter documentation here!

#ifndef VTKAMRBASEPARTICLESREADER_H_
#define VTKAMRBASEPARTICLESREADER_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

class VTK_AMR_EXPORT vtkAMRBaseParticlesReader :
  public vtkMultiBlockDataSetAlgorithm
{
  public:
      vtkTypeMacro( vtkAMRBaseParticlesReader, vtkMultiBlockDataSetAlgorithm );
      void PrintSelf( std::ostream &os, vtkIndent indent );

      vtkGetMacro(Frequency,int);
      vtkSetMacro(Frequency,int);

  protected:
    vtkAMRBaseParticlesReader();
    virtual ~vtkAMRBaseParticlesReader();

    // Description:
    // Initializes the AMR Particles reader
    // NOTE: must be called in the constructor of concrete classes.
    void Initialize();

    int Frequency;
  private:
    vtkAMRBaseParticlesReader( const vtkAMRBaseParticlesReader& ); // Not implemented
    void operator=(const vtkAMRBaseParticlesReader& ); // Not implemented
};

#endif /* VTKAMRBASEPARTICLESREADER_H_ */
