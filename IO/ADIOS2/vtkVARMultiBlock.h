/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkVARMultiBlock.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * vtkVARMultiBlock.h  public facing class
 *                     enables reading adios2 bp files using the
 *                     VTK ADIOS2 Readers (VAR) developed
 *                     at Oak Ridge National Laboratory
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef vtkVARMultiBlock_h
#define vtkVARMultiBlock_h

#include <memory> // std::unique_ptr

#include "vtkIOADIOS2Module.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

// forward declaring to keep it private
namespace var
{
class VARSchemaManager;
}

class vtkIndent;
class vtkInformation;
class vtkInformationvector;

class VTKIOADIOS2_EXPORT vtkVARMultiBlock : public vtkMultiBlockDataSetAlgorithm
{
public:
    static vtkVARMultiBlock *New();
    vtkTypeMacro(vtkVARMultiBlock, vtkMultiBlockDataSetAlgorithm);
    void PrintSelf(ostream &os, vtkIndent index) override;

    vtkSetStringMacro(FileName);
    vtkGetStringMacro(FileName);

protected:
    vtkVARMultiBlock();
    ~vtkVARMultiBlock() = default;

    vtkVARMultiBlock(const vtkVARMultiBlock &) = delete;
    void operator=(const vtkVARMultiBlock &) = delete;

    int RequestInformation(vtkInformation *, vtkInformationVector **,
                           vtkInformationVector *outputVector);
    int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                            vtkInformationVector *outputVector);
    int RequestData(vtkInformation *, vtkInformationVector **,
                    vtkInformationVector *outputVector);

private:
    char *FileName;
    std::unique_ptr<var::VARSchemaManager> SchemaManager;
};

#endif /* vtkVARMultiBlock_h */
