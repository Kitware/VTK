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
/**
 * @class   vtkFieldDataSerializer
 *
 *
 *  A concrete instance of vtkObject which provides functionality for
 *  serializing and de-serializing field data, primarily used for the purpose
 *  of preparing the data for transfer over MPI or other communication
 *  mechanism.
 *
 * @sa
 * vtkFieldData vtkPointData vtkCellData vtkMultiProcessStream
*/

#ifndef vtkFieldDataSerializer_h
#define vtkFieldDataSerializer_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"

// Forward declarations
class vtkIdList;
class vtkFieldData;
class vtkDataArray;
class vtkStringArray;
class vtkIntArray;
class vtkMultiProcessStream;

class VTKPARALLELCORE_EXPORT vtkFieldDataSerializer : public vtkObject
{
  public:
    static vtkFieldDataSerializer* New();
    vtkTypeMacro(vtkFieldDataSerializer,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /**
     * Serializes the metadata of the given field data instance, i.e., the
     * number of arrays, the name of each array and their dimensions.
     */
    static void SerializeMetaData(
        vtkFieldData *fieldData, vtkMultiProcessStream& bytestream);

    /**
     * Given the serialized field metadata in a bytestream, this method extracts
     * the name, datatype and dimensions of each array. The metadata is
     * deserialized on user-supplied, pre-allocated data structures.
     * (1) names -- an array of strings wherein, each element, names[i],
     * corresponds to the name of array i.
     * (2) datatypes -- an array of ints where each element corresponds
     * to the actual primitive type of each array, e.g.,VTK_DOUBLE,VTK_INT, etc.
     * (3) dimensions -- a 2-component array of integers where the first
     * component corresponds to the number of tuples of and the second component
     * corresponds to the number components of array i.
     */
    static void DeserializeMetaData(
        vtkMultiProcessStream& bytestream,
        vtkStringArray *names,
        vtkIntArray *datatypes,
        vtkIntArray *dimensions);

    /**
     * Serializes the given field data (all the field data) into a bytestream.
     */
    static void Serialize(
        vtkFieldData *fieldData, vtkMultiProcessStream& bytestream);

    /**
     * Serializes the selected tuples from the field data in a byte-stream.
     */
    static void SerializeTuples(
        vtkIdList *tupleIds, vtkFieldData *fieldData,
        vtkMultiProcessStream& bytestream);

    /**
     * Serializes the given sub-extent of field data of a structured grid
     * in a byte-stream. The field data can be either cell-centered or
     * node-centered depending on what subext and gridExtent actually
     * represents.
     */
    static void SerializeSubExtent(
        int subext[6], int gridExtent[6], vtkFieldData *fieldData,
        vtkMultiProcessStream& bytestream);

    /**
     * Deserializes the field data from a bytestream to a the given sub-extent.
     * The field data can be either cell-centered or node-centered depending
     * on what subext and gridExtent actually represent.
     */
    static void DeSerializeToSubExtent(
        int subext[6], int gridExtent[6], vtkFieldData *fieldData,
        vtkMultiProcessStream& bytestream);

    /**
     * Deserializes the field data from a bytestream.
     */
    static void Deserialize(
        vtkMultiProcessStream& bytestream, vtkFieldData *fieldData );

  protected:
    vtkFieldDataSerializer();
    ~vtkFieldDataSerializer() override;

    /**
     * Given an input data array and list of tuples, it extracts the selected
     * tuples in to a new array and returns it.
     */
    static vtkDataArray* ExtractSelectedTuples(
        vtkIdList *tupleIds, vtkDataArray *inputDataArray );

    /**
     * Given an input data array corresponding to a field on a structured grid,
     * extract the data within the given extent.
     */
    static vtkDataArray* ExtractSubExtentData(
        int subext[6], int gridExtent[6], vtkDataArray *inputDataArray);

    /**
     * Serializes the data array into a bytestream.
     */
    static void SerializeDataArray(
        vtkDataArray *dataArray, vtkMultiProcessStream& bytestream );

    /**
     * Deserializes the data array from a bytestream
     */
    static void DeserializeDataArray(
        vtkMultiProcessStream& bytestream, vtkDataArray *&dataArray );


  private:
    vtkFieldDataSerializer(const vtkFieldDataSerializer&) = delete;
    void operator=(const vtkFieldDataSerializer&) = delete;
};

#endif /* vtkFieldDataSerializer_h */
