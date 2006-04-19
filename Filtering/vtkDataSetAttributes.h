/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetAttributes - represent and manipulate attribute data in a dataset
// .SECTION Description
// vtkDataSetAttributes is a class that is used to represent and manipulate
// attribute data (e.g., scalars, vectors, normals, texture coordinates,
// tensors, and field data) Special methods are provided to work with filter
// objects, such as passing data through filter, copying data from one
// attribute set to another, and interpolating data given cell interpolation
// weights.

#ifndef __vtkDataSetAttributes_h
#define __vtkDataSetAttributes_h

#include "vtkFieldData.h"

class VTK_FILTERING_EXPORT vtkDataSetAttributes : public vtkFieldData
{
public:
  // Description:
  // Construct object with copying turned on for all data.
  static vtkDataSetAttributes *New();
  
  vtkTypeRevisionMacro(vtkDataSetAttributes,vtkFieldData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize all of the object's data to NULL
  // Also, clear the copy flags.
  virtual void Initialize();

  // Description:
  // Attributes have a chance to bring themselves up to date; right
  // now this is ignored.
  virtual void Update() {}

  // Description:
  // Pass entire arrays of input data through to output. Obey the "copy"
  // flags. When passing a field,  the following copying rules are 
  // followed: 1) Check if a field is an attribute, if yes and if there
  // is a copy flag for that attribute (on or off), obey the  flag for 
  // that attribute, ignore (2) and (3), 2) if there is a copy field for
  // that field (on or off), obey the flag, ignore (3) 3) obey
  // CopyAllOn/Off
  virtual void PassData(vtkFieldData* fd);

  // Description:
  // Allocates point data for point-by-point (or cell-by-cell) copy operation.
  // If sze=0, then use the input DataSetAttributes to create (i.e., find 
  // initial size of) new objects; otherwise use the sze variable.
  // Note that pd HAS to be the vtkDataSetAttributes object which
  // will later be used with CopyData. If this is not the case,
  // consider using the alternative forms of CopyAllocate and CopyData.
  // ext is no longer used.
  void CopyAllocate(vtkDataSetAttributes* pd, vtkIdType sze=0,
                    vtkIdType ext=1000);

  // Description:
  // Copy the attribute data from one id to another. Make sure CopyAllocate()
  // has been invoked before using this method. When copying a field,  
  // the following copying rules are 
  // followed: 1) Check if a field is an attribute, if yes and if there
  // is a copy flag for that attribute (on or off), obey the  flag for 
  // that attribute, ignore (2) and (3), 2) if there is a copy field for
  // that field (on or off), obey the flag, ignore (3) 3) obey
  // CopyAllOn/Off
  void CopyData(vtkDataSetAttributes *fromPd, vtkIdType fromId, vtkIdType toId);

  // Description:
  // Initialize point interpolation method.
  // Note that pd HAS to be the vtkDataSetAttributes object which
  // will later be used with InterpolatePoint or InterpolateEdge.
  // ext is no longer used.
  void InterpolateAllocate(vtkDataSetAttributes* pd, vtkIdType sze=0,
                           vtkIdType ext=1000);
  
  // Description:
  // This method is used to copy data arrays in images.
  // You should not call "CopyAllocate" before calling this method.
  // This method is called once to copy all of the data.
  // If the two extents are the same, this method calls "PassData".
  void CopyStructuredData(vtkDataSetAttributes *inDsa,
                          const int *inExt, const int *outExt);

  // Description:
  // Interpolate data set attributes from other data set attributes
  // given cell or point ids and associated interpolation weights.
  void InterpolatePoint(vtkDataSetAttributes *fromPd, vtkIdType toId, 
                        vtkIdList *ids, double *weights);
  
  // Description:
  // Interpolate data from the two points p1,p2 (forming an edge) and an 
  // interpolation factor, t, along the edge. The weight ranges from (0,1), 
  // with t=0 located at p1. Make sure that the method InterpolateAllocate() 
  // has been invoked before using this method.
  void InterpolateEdge(vtkDataSetAttributes *fromPd, vtkIdType toId,
                       vtkIdType p1, vtkIdType p2, double t);

  // Description:
  // Interpolate data from the same id (point or cell) at different points
  // in time (parameter t). Two input data set attributes objects are input.
  // The parameter t lies between (0<=t<=1). IMPORTANT: it is assumed that
  // the number of attributes and number of components is the same for both
  // from1 and from2, and the type of data for from1 and from2 are the same.
  // Make sure that the method InterpolateAllocate() has been invoked before 
  // using this method.
  void InterpolateTime(vtkDataSetAttributes *from1, 
                       vtkDataSetAttributes *from2,
                       vtkIdType id, double t);

  // Description:
  // Deep copy of data (i.e., create new data arrays and
  // copy from input data).
  virtual void DeepCopy(vtkFieldData *pd);

  // Description:
  // Shallow copy of data (i.e., use reference counting).
  virtual void ShallowCopy(vtkFieldData *pd);

  // Description:
  // Set/Get the scalar data.
  int SetScalars(vtkDataArray* da);
  int SetActiveScalars(const char* name);
  vtkDataArray* GetScalars();

  // Description:
  // Set/Get the vector data.
  int SetVectors(vtkDataArray* da);
  int SetActiveVectors(const char* name);
  vtkDataArray* GetVectors();

  // Description:
  // Set/get the normal data.
  int SetNormals(vtkDataArray* da);
  int SetActiveNormals(const char* name);
  vtkDataArray* GetNormals();

  // Description:
  // Set/Get the texture coordinate data.
  int SetTCoords(vtkDataArray* da);
  int SetActiveTCoords(const char* name);
  vtkDataArray* GetTCoords();

  // Description:
  // Set/Get the tensor data.
  int SetTensors(vtkDataArray* da);
  int SetActiveTensors(const char* name);
  vtkDataArray* GetTensors();

  // Description:
  // This will first look for an array with the correct name.
  // If one exists, it is returned. Otherwise, the name argument
  // is ignored, and the active attribute is returned.
  vtkDataArray* GetScalars(const char* name);
  vtkDataArray* GetVectors(const char* name);
  vtkDataArray* GetNormals(const char* name);
  vtkDataArray* GetTCoords(const char* name);
  vtkDataArray* GetTensors(const char* name);

  // Description:
  // Make the array with the given name the active attribute.
  // Attribute types are:
  //  vtkDataSetAttributes::SCALARS = 0
  //  vtkDataSetAttributes::VECTORS = 1
  //  vtkDataSetAttributes::NORMALS = 2
  //  vtkDataSetAttributes::TCOORDS = 3
  //  vtkDataSetAttributes::TENSORS = 4
  // Returns the index of the array if succesful, -1 if the array 
  // is not in the list of arrays.
  int SetActiveAttribute(const char* name, int attributeType);

  // Description:
  // Make the array with the given index the active attribute.
  int SetActiveAttribute(int index, int attributeType);

  // Description:
  // Specify whether to copy the data attribute referred to by index i.
  void SetCopyAttribute (int index, int value);

  // Description:
  // Turn on/off the copying of scalar data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyScalars(int i);
  int GetCopyScalars();
  vtkBooleanMacro(CopyScalars, int);

  // Description:
  // Turn on/off the copying of vector data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyVectors(int i);
  int GetCopyVectors();
  vtkBooleanMacro(CopyVectors, int);

  // Description:
  // Turn on/off the copying of normals data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyNormals(int i);
  int GetCopyNormals();
  vtkBooleanMacro(CopyNormals, int);

  // Description:
  // Turn on/off the copying of texture coordinates data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyTCoords(int i);
  int GetCopyTCoords();
  vtkBooleanMacro(CopyTCoords, int);

  // Description:
  // Turn on/off the copying of tensor data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyTensors(int i);
  int GetCopyTensors();
  vtkBooleanMacro(CopyTensors, int);

  // Description:
  // Turn on copying of all data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOn();

  // Description:
  // Turn off copying of all data.
  // During the copy/pass, the following rules are followed for each
  // array:
  // 1. If the copy flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOff();

  // Description:
  // Copy a tuple of data from one data array to another. This method (and
  // following ones) assume that the fromData and toData objects are of the
  // same type, and have the same number of components. This is true if you
  // invoke CopyAllocate() or InterpolateAllocate().
  void CopyTuple(vtkDataArray *fromData, vtkDataArray *toData, 
                 vtkIdType fromId, vtkIdType toId);

  // Description:
  // Get the field data array indices corresponding to scalars, 
  // vectors, tensors, etc.
  void GetAttributeIndices(int* indexArray);

  // Description:
  // Determine whether a data array of index idx is considered a data set
  // attribute (i.e., scalar, vector, tensor, etc). Return less-than zero 
  // if it is, otherwise an index 0<=idx<NUM_ATTRIBUTES to indicate 
  // which attribute.
  int IsArrayAnAttribute(int idx);

  // Description:
  // Return an attribute given the attribute type
  // (see vtkDataSetAttributes::AttributeTypes).
  vtkDataArray* GetAttribute(int attributeType);

  // Description:
  // Remove an array (with the given name) from the list of arrays.
  virtual void RemoveArray(const char *name);

  // Description:
  // Given an integer attribute type, this static method returns a string type
  // for the attribute (i.e. type = 0: returns "Scalars").
  static const char* GetAttributeTypeAsString(int attributeType);
  static const char* GetLongAttributeTypeAsString(int attributeType);

//BTX
  // Always keep NUM_ATTRIBUTES as the last entry
  enum AttributeTypes 
  {
    SCALARS=0,
    VECTORS=1,
    NORMALS=2,
    TCOORDS=3,
    TENSORS=4,
    NUM_ATTRIBUTES
  };

  enum AttributeLimitTypes 
  {
    MAX, 
    EXACT, 
    NOLIMIT
  };

  class FieldList;

  // Description:
  // A special form of CopyAllocate() to be used with FieldLists. Use it 
  // when you are copying data from a set of vtkDataSetAttributes.
  void CopyAllocate(vtkDataSetAttributes::FieldList& list, vtkIdType sze=0, 
                    vtkIdType ext=1000);

  // Description:
  // A special form of CopyData() to be used with FieldLists. Use it when 
  // you are copying data from a set of vtkDataSetAttributes. Make sure 
  // that you have called the special form of CopyAllocate that accepts 
  // FieldLists.
  void CopyData(vtkDataSetAttributes::FieldList& list, 
                vtkDataSetAttributes* dsa, int idx, vtkIdType fromId,
                vtkIdType toId);

  friend class vtkDataSetAttributes::FieldList;
//ETX

protected:
  vtkDataSetAttributes();
  ~vtkDataSetAttributes();

  // special methods to support managing data
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData,
                        vtkIdType toId, vtkIdList *ptIds, double *weights);
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData,
                        vtkIdType toId, vtkIdType id1, vtkIdType id2, 
                        double t);
  void InterpolateTuple(vtkDataArray *fromData1, vtkDataArray *fromData2, 
                        vtkDataArray *toData, vtkIdType id, double t);

  // Description:
  // Initialize all of the object's data to NULL
  virtual void InitializeFields();

  int AttributeIndices[NUM_ATTRIBUTES]; //index to attribute array in field data
  int CopyAttributeFlags[NUM_ATTRIBUTES]; //copy flag for attribute data

//BTX
  vtkFieldData::BasicIterator RequiredArrays;
//ETX

  int* TargetIndices;

  virtual void RemoveArray(int index);

  static const int NumberOfAttributeComponents[NUM_ATTRIBUTES];
  static const int AttributeLimits[NUM_ATTRIBUTES];
  static const char AttributeNames[NUM_ATTRIBUTES][10];
  static const char LongAttributeNames[NUM_ATTRIBUTES][35];

private:
  int SetAttribute(vtkDataArray* da, int attributeType);
  static int CheckNumberOfComponents(vtkDataArray* da, int attributeType);

//BTX
  vtkFieldData::BasicIterator  ComputeRequiredArrays(vtkDataSetAttributes* pd);

private:
  vtkDataSetAttributes(const vtkDataSetAttributes&);  // Not implemented.
  void operator=(const vtkDataSetAttributes&);  // Not implemented.

public:
  // This public class is used to perform set operations, other misc. 
  // operations on fields. For example, vtkAppendFilter uses it to 
  // determine which attributes the input datasets share in common.
  class VTK_FILTERING_EXPORT FieldList
  {
  public:
    FieldList(int numInputs);
    ~FieldList();

    void InitializeFieldList(vtkDataSetAttributes* dsa);
    void IntersectFieldList(vtkDataSetAttributes* dsa);

    //Determine whether data is available
    int IsAttributePresent(int attrType); //true/false attributes specified
    
    // Accessor methods.
    int GetNumberOfFields() { return this->NumberOfFields; }
    int GetFieldIndex(int i) { return this->FieldIndices[i]; }
    int GetDSAIndex(int index, int i) { return this->DSAIndices[index][i]; }
    
    friend class vtkDataSetAttributes;

  protected:
    FieldList(const FieldList&) {} //prevent these methods from being used
    void operator=(const FieldList&) {}

  private:
    void SetField(int index, vtkDataArray *da);
    void RemoveField(const char *name);
    void ClearFields();
    
    //These keep track of what is common across datasets
    char** Fields; //the names of the fields (first five are named attributes)
    int *FieldTypes; //the types of the fields (first five are named 
                     //attributes)
    int *FieldComponents; //the number of components in each  fields 
                          // (first five are named attributes)
    int *FieldIndices; //output data array index 
                       // (first five are named attributes)
    vtkLookupTable **LUT; //luts associated with each array
    vtkIdType NumberOfTuples; //a running total of values
    int NumberOfFields; //the number of fields
    
    //For every vtkDataSetAttributes that are processed, keep track of the 
    //indices into various things. The indices are organized so that the
    //first NUM_ATTRIBUTES refer to attributes, the next refer to the 
    //non-attribute fields, for a total of NUM_ATTRIBUTES + NumberOfFields.
    //CurrentInput is the current input being processed.
    int **DSAIndices;
    int NumberOfDSAIndices;
    int CurrentInput;
  };

//ETX
};

#endif


