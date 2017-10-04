/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimplePointsWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimplePointsWriter
 * @brief   write a file of xyz coordinates
 *
 * vtkSimplePointsWriter writes a simple file of xyz coordinates
 *
 * @sa
 * vtkSimplePointsReader
*/

#ifndef vtkSimplePointsWriter_h
#define vtkSimplePointsWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataSetWriter.h"

class VTKIOLEGACY_EXPORT vtkSimplePointsWriter : public vtkDataSetWriter
{
public:
  static vtkSimplePointsWriter *New();
  vtkTypeMacro(vtkSimplePointsWriter,vtkDataSetWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetMacro(DecimalPrecision, int);
  vtkSetMacro(DecimalPrecision, int);

protected:
  vtkSimplePointsWriter();
  ~vtkSimplePointsWriter() override{}

  void WriteData() override;

  int DecimalPrecision;

private:
  vtkSimplePointsWriter(const vtkSimplePointsWriter&) = delete;
  void operator=(const vtkSimplePointsWriter&) = delete;
};

#endif
