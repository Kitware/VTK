/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkFieldDataSerializer.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkFieldDataSerializer.h -- Field data serialization/de-serialization
//
// .SECTION Description
//  A concrete instance of vtkObject which provides functionality for
//  serializing and de-serializing field data, primarily used for the purpose
//  of preparing the data for transfer over MPI or other communication
//  mechanism.
//
// .SECTION See Also
// vtkFieldData vtkPointData vtkCellData vtkMultiProcessStream

#ifndef VTKFIELDDATASERIALIZER_H_
#define VTKFIELDDATASERIALIZER_H_

#include "vtkObject.h"

// Forward declarations
class vtkIdList;
class vtkFieldData;
class vtkDataArray;
class vtkMultiProcessStream;

class VTK_PARALLEL_EXPORT vtkFieldDataSerializer : public vtkObject
{
  public:
    static vtkFieldDataSerializer* New();
    vtkTypeMacro(vtkFieldDataSerializer,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Serializes the given field data (all the field data) into a bytestream.
    static void Serialize(
        vtkFieldData *fieldData, vtkMultiProcessStream& bytestream);

    // Description:
    // Serializes the selected tuples from the the field data in a byte-stream.
    static void SerializeTuples(
        vtkIdList *tupleIds, vtkFieldData *fieldData,
        vtkMultiProcessStream& bytestream);

    // Description:
    // Serializes the given sub-extent of field data of a structured grid
    // in a byte-stream. The field data can be either cell-centered or
    // node-centered depending on what subext and gridExtent actually
    // represents.
    static void SerializeSubExtent(
        int subext[6], int gridExtent[6], vtkFieldData *fieldData,
        vtkMultiProcessStream& bytestream);

    // Description:
    // Deserializes the field data from a bytestream.
    static void Deserialize(
        vtkFieldData *fieldData, vtkMultiProcessStream& bytestream);

  protected:
    vtkFieldDataSerializer();
    virtual ~vtkFieldDataSerializer();

    // Description:
    // Given an input data array and list of tuples, it extracts the selected
    // tuples in to a new array and returns it.
    static vtkDataArray* ExtractSelectedTuples(
        vtkIdList *tupleIds, vtkDataArray *inputDataArray );

    // Description:
    // Given an input data array corresponding to a field on a structured grid,
    // extract the data within the given extent.
    static vtkDataArray* ExtractSubExtentData(
        int subext[6], int gridExtent[6], vtkDataArray *inputDataArray);

    // Description:
    // Serializes the data array into a bytestream.
    static void SerializeDataArray(
        vtkDataArray *dataArray, vtkMultiProcessStream& bytestream );

    // Description:
    // Deserializes the data array from a bytestream
    static void DeserializeDataArray(
        vtkDataArray *&dataArray, vtkMultiProcessStream& bytestream );


  private:
    vtkFieldDataSerializer(const vtkFieldDataSerializer&); // Not implemented
    void operator=(const vtkFieldDataSerializer&); // Not implemented
};

#endif /* VTKFIELDDATASERIALIZER_H_ */
