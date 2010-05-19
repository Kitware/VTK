/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLStructuredDataWriter - Superclass for VTK XML structured data writers.
// .SECTION Description
// vtkXMLStructuredDataWriter provides VTK XML writing functionality that
// is common among all the structured data formats.

#ifndef __vtkXMLStructuredDataWriter_h
#define __vtkXMLStructuredDataWriter_h

#include "vtkXMLWriter.h"

class vtkAbstractArray;
class vtkExtentTranslator;
class vtkInformation;
class vtkInformationVector;

class VTK_IO_EXPORT vtkXMLStructuredDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLStructuredDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the number of pieces used to stream the image through the
  // pipeline while writing to the file.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set the extent of the input that should be treated as the
  // WholeExtent in the output file.  The default is the WholeExtent
  // of the input.
  vtkSetVector6Macro(WriteExtent, int);
  vtkGetVector6Macro(WriteExtent, int);
  
  // Description:
  // Get/Set the extent translator used for streaming.
  virtual void SetExtentTranslator(vtkExtentTranslator*);
  vtkGetObjectMacro(ExtentTranslator, vtkExtentTranslator);
  
protected:
  vtkXMLStructuredDataWriter();
  ~vtkXMLStructuredDataWriter();  
  
  // Writing drivers defined by subclasses.
  virtual void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  virtual void WriteAppendedPiece(int index, vtkIndent indent);
  virtual void WriteAppendedPieceData(int index);
  virtual void WriteInlinePiece(vtkIndent indent);
  virtual void GetInputExtent(int* extent)=0;
  
  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  void SetupExtentTranslator();
  vtkAbstractArray* CreateExactExtent(vtkAbstractArray* array, int* inExtent,
                                  int* outExtent, int isPoint);
  virtual int WriteInlineMode(vtkIndent indent);
  vtkIdType GetStartTuple(int* extent, vtkIdType* increments,
                          int i, int j, int k);
  void CalculatePieceFractions(float* fractions);
  
  // Define utility methods required by vtkXMLWriter.
  virtual vtkAbstractArray* CreateArrayForPoints(vtkAbstractArray* inArray);
  virtual vtkAbstractArray* CreateArrayForCells(vtkAbstractArray* inArray);
  
  void SetInputUpdateExtent(int piece);
  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector);

  vtkSetVector6Macro(InternalWriteExtent, int);

  // The extent of the input to write, as specified by user
  int WriteExtent[6];
  
  // The actual extent of the input to write.
  int InternalWriteExtent[6];
  
  // Number of pieces used for streaming.
  int NumberOfPieces;
  
  // Translate piece number to extent.
  vtkExtentTranslator* ExtentTranslator;

  float* ProgressFractions;

  int CurrentPiece;

  // Appended data offsets of point and cell data arrays.
  // Store offset position (add TimeStep support)
  OffsetsManagerArray *PointDataOM;
  OffsetsManagerArray *CellDataOM;
 
private:
  vtkXMLStructuredDataWriter(const vtkXMLStructuredDataWriter&);  // Not implemented.
  void operator=(const vtkXMLStructuredDataWriter&);  // Not implemented.
};

#endif
