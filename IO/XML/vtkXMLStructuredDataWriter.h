// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLStructuredDataWriter
 * @brief   Superclass for VTK XML structured data writers.
 *
 * vtkXMLStructuredDataWriter provides VTK XML writing functionality that
 * is common among all the structured data formats.
 */

#ifndef vtkXMLStructuredDataWriter_h
#define vtkXMLStructuredDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkInformation;
class vtkInformationVector;

class VTKIOXML_EXPORT vtkXMLStructuredDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLStructuredDataWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the number of pieces used to stream the image through the
   * pipeline while writing to the file.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  ///@}

  ///@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative, all pieces will be written.
   */
  vtkSetMacro(WritePiece, int);
  vtkGetMacro(WritePiece, int);
  ///@}

  ///@{
  /**
   * Get/Set the ghost level used to pad each piece.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * Get/Set the extent of the input that should be treated as the
   * WholeExtent in the output file.  The default is the WholeExtent
   * of the input.
   */
  vtkSetVector6Macro(WriteExtent, int);
  vtkGetVector6Macro(WriteExtent, int);
  ///@}

protected:
  vtkXMLStructuredDataWriter();
  ~vtkXMLStructuredDataWriter() override;

  // Writing drivers defined by subclasses.
  void WritePrimaryElementAttributes(ostream& os, vtkIndent indent) override;
  virtual void WriteAppendedPiece(int index, vtkIndent indent);
  virtual void WriteAppendedPieceData(int index);
  virtual void WriteInlinePiece(vtkIndent indent);
  virtual void GetInputExtent(int* extent) = 0;

  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  virtual int WriteInlineMode(vtkIndent indent);
  vtkIdType GetStartTuple(int* extent, vtkIdType* increments, int i, int j, int k);
  void CalculatePieceFractions(float* fractions);

  void SetInputUpdateExtent(int piece);
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkSetVector6Macro(InternalWriteExtent, int);

  static vtkIdType GetNumberOfValues(vtkDataSet* input);

  // The extent of the input to write, as specified by user
  int WriteExtent[6];

  // The actual extent of the input to write.
  int InternalWriteExtent[6];

  // Number of pieces used for streaming.
  int NumberOfPieces;

  int WritePiece;

  float* ProgressFractions;

  int CurrentPiece;

  int GhostLevel;

  vtkTypeInt64* ExtentPositions;

  // Appended data offsets of point and cell data arrays.
  // Store offset position (add TimeStep support)
  OffsetsManagerArray* PointDataOM;
  OffsetsManagerArray* CellDataOM;

private:
  vtkXMLStructuredDataWriter(const vtkXMLStructuredDataWriter&) = delete;
  void operator=(const vtkXMLStructuredDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
