/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPartitionedDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkXMLPPartitionedDataSetWriter
 * @brief deprecated. Simply use vtkXMLPartitionedDataSetWriter.
 *
 * vtkXMLPartitionedDataSetWriter is deprecated. Simple use
 * vtkXMLPartitionedDataSetWriter.
 */

#ifndef vtkXMLPPartitionedDataSetWriter_h
#define vtkXMLPPartitionedDataSetWriter_h

#include "vtkDeprecation.h"         // For VTK_DEPRECATED_IN_9_1_0
#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPartitionedDataSetWriter.h"

class vtkCompositeDataSet;
class vtkMultiProcessController;

VTK_DEPRECATED_IN_9_1_0("Use vtkXMLPartitionedDataSetWriter instead")
class VTKIOPARALLELXML_EXPORT vtkXMLPPartitionedDataSetWriter
  : public vtkXMLPartitionedDataSetWriter
{
public:
  static vtkXMLPPartitionedDataSetWriter* New();
  vtkTypeMacro(vtkXMLPPartitionedDataSetWriter, vtkXMLPartitionedDataSetWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * These no longer have any effect. Only left for backwards compatibility.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  void SetWriteMetaFile(int) {}
  ///@}

protected:
  vtkXMLPPartitionedDataSetWriter();
  ~vtkXMLPPartitionedDataSetWriter() override;

  ///@{
  /**
   * Piece information.
   */
  int StartPiece = 0;
  int NumberOfPieces = 1;
  ///@}

private:
  vtkXMLPPartitionedDataSetWriter(const vtkXMLPPartitionedDataSetWriter&) = delete;
  void operator=(const vtkXMLPPartitionedDataSetWriter&) = delete;
};

#endif
