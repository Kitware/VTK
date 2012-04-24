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
// tensors, global ids, pedigree ids, and field data).
//
// This adds to vtkFieldData the ability to pick one of the arrays from the
// field as the currently active array for each attribute type. In other
// words, you pick one array to be called "THE" Scalars, and then filters down
// the pipeline will treat that array specially. For example vtkContourFilter
// will contour "THE" Scalar array unless a different array is asked for.
//
// Additionally vtkDataSetAttributes provides methods that filters call to
// pass data through, copy data into, and interpolate from Fields. PassData
// passes entire arrays from the source to the destination. Copy passes
// through some subset of the tuples from the source to the destination.
// Interpolate interpolates from the chosen tuple(s) in the source data, using
// the provided weights, to produce new tuples in the destination.
// Each attribute type has pass, copy and interpolate "copy" flags that
// can be set in the destination to choose which attribute arrays will be
// transferred from the source to the destination.
//
// Finally this class provides a mechanism to determine which attributes a
// group of sources have in common, and to copy tuples from a source into
// the destination, for only those attributes that are held by all.

#ifndef __vtkDataSetAttributes_h
#define __vtkDataSetAttributes_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkFieldData.h"

class vtkLookupTable;

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetAttributes : public vtkFieldData
{
public:
  // Description:
  // Construct object with copying turned on for all data.
  static vtkDataSetAttributes *New();

  vtkTypeMacro(vtkDataSetAttributes,vtkFieldData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize all of the object's data to NULL
  // Also, clear the copy flags.
  virtual void Initialize();

  // Description:
  // Attributes have a chance to bring themselves up to date; right
  // now this is ignored.
  virtual void Update() {}

  // -- shallow and deep copy -----------------------------------------------

  // Description:
  // Deep copy of data (i.e., create new data arrays and
  // copy from input data).
  // Ignores the copy flags but preserves them in the output.
  virtual void DeepCopy(vtkFieldData *pd);

  // Description:
  // Shallow copy of data (i.e., use reference counting).
  // Ignores the copy flags but preserves them in the output.
  virtual void ShallowCopy(vtkFieldData *pd);

  // -- attribute types -----------------------------------------------------
//BTX
  // Always keep NUM_ATTRIBUTES as the last entry
  enum AttributeTypes
  {
    SCALARS=0,
    VECTORS=1,
    NORMALS=2,
    TCOORDS=3,
    TENSORS=4,
    GLOBALIDS=5,
    PEDIGREEIDS=6,
    EDGEFLAG=7,
    NUM_ATTRIBUTES
  };

  enum AttributeLimitTypes
  {
    MAX,
    EXACT,
    NOLIMIT
  };
//ETX

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
  // Set/Get the global id data.
  int SetGlobalIds(vtkDataArray* da);
  int SetActiveGlobalIds(const char* name);
  vtkDataArray* GetGlobalIds();

  // Description:
  // Set/Get the pedigree id data.
  int SetPedigreeIds(vtkAbstractArray* da);
  int SetActivePedigreeIds(const char* name);
  vtkAbstractArray* GetPedigreeIds();

  // Description:
  // This will first look for an array with the correct name.
  // If one exists, it is returned. Otherwise, the name argument
  // is ignored, and the active attribute is returned.
  vtkDataArray* GetScalars(const char* name);
  vtkDataArray* GetVectors(const char* name);
  vtkDataArray* GetNormals(const char* name);
  vtkDataArray* GetTCoords(const char* name);
  vtkDataArray* GetTensors(const char* name);
  vtkDataArray* GetGlobalIds(const char* name);
  vtkAbstractArray* GetPedigreeIds(const char* name);

  // Description:
  // Make the array with the given name the active attribute.
  // Attribute types are:
  //  vtkDataSetAttributes::SCALARS = 0
  //  vtkDataSetAttributes::VECTORS = 1
  //  vtkDataSetAttributes::NORMALS = 2
  //  vtkDataSetAttributes::TCOORDS = 3
  //  vtkDataSetAttributes::TENSORS = 4
  //  vtkDataSetAttributes::GLOBALIDS = 5
  //  vtkDataSetAttributes::PEDIGREEIDS = 6
  //  vtkDataSetAttributes::EDGEFLAG = 7
  // Returns the index of the array if successful, -1 if the array
  // is not in the list of arrays.
  int SetActiveAttribute(const char* name, int attributeType);

  // Description:
  // Make the array with the given index the active attribute.
  int SetActiveAttribute(int index, int attributeType);

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
  // Some attributes (such as PEDIGREEIDS) may not be vtkDataArray subclass,
  // so in that case use GetAbstractAttribute().
  vtkDataArray* GetAttribute(int attributeType);

  // Description:
  // Return an attribute given the attribute type
  // (see vtkDataSetAttributes::AttributeTypes).
  // This is the same as GetAttribute(), except that the returned array
  // is a vtkAbstractArray instead of vtkDataArray.
  // Some attributes (such as PEDIGREEIDS) may not be vtkDataArray subclass.
  vtkAbstractArray* GetAbstractAttribute(int attributeType);

  // Description:
  // Remove an array (with the given name) from the list of arrays.
  virtual void RemoveArray(const char *name);
  virtual void RemoveArray(int index);


  // Description:
  // Given an integer attribute type, this static method returns a string type
  // for the attribute (i.e. type = 0: returns "Scalars").
  static const char* GetAttributeTypeAsString(int attributeType);
  static const char* GetLongAttributeTypeAsString(int attributeType);

  // -- attribute copy properties ------------------------------------------

//BTX
  enum AttributeCopyOperations
  {
    COPYTUPLE=0,
    INTERPOLATE=1,
    PASSDATA=2,
    ALLCOPY  //all of the above
  };
//ETX

  // Description:
  // Specify whether to copy the data attribute referred to by index.
  // ctype selects from the AttributeCopyOperations.
  // If ctype is set to ALLCOPY, then COPYTUPLE, INTERPOLATE, and
  // PASSDATA are set to value. If value is 0, copying is disallowed.
  // otherwise it is allowed.
  void SetCopyAttribute (int index, int value, int ctype=ALLCOPY);

  // Description:
  // Turn on/off the copying of scalar data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyScalars(int i, int ctype=ALLCOPY);
  int GetCopyScalars(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyScalars, int);

  // Description:
  // Turn on/off the copying of vector data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyVectors(int i, int ctype=ALLCOPY);
  int GetCopyVectors(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyVectors, int);

  // Description:
  // Turn on/off the copying of normals data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyNormals(int i, int ctype=ALLCOPY);
  int GetCopyNormals(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyNormals, int);

  // Description:
  // Turn on/off the copying of texture coordinates data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyTCoords(int i, int ctype=ALLCOPY);
  int GetCopyTCoords(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyTCoords, int);

  // Description:
  // Turn on/off the copying of tensor data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyTensors(int i, int ctype=ALLCOPY);
  int GetCopyTensors(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyTensors, int);

  // Description:
  // Turn on/off the copying of global id data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyGlobalIds(int i, int ctype=ALLCOPY);
  int GetCopyGlobalIds(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyGlobalIds, int);

  // Description:
  // Turn on/off the copying of pedigree id data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  void SetCopyPedigreeIds(int i, int ctype=ALLCOPY);
  int GetCopyPedigreeIds(int ctype=ALLCOPY);
  vtkBooleanMacro(CopyPedigreeIds, int);

  // Description:
  // Turn on copying of all data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOn(int ctype=ALLCOPY);

  // Description:
  // Turn off copying of all data.
  // ctype is one of the AttributeCopyOperations, and controls copy,
  // interpolate and passdata behavior.
  // For set, ctype=ALLCOPY means set all three flags to the same value.
  // For get, ctype=ALLCOPY returns true only if all three flags are true.
  //
  // During copying, interpolation and passdata, the following rules are
  // followed for each array:
  // 1. If the copy/interpolate/pass flag for an attribute is set (on or off), it is applied.
  //    This overrides rules 2 and 3.
  // 2. If the copy flag for an array is set (on or off), it is applied
  //    This overrides rule 3.
  // 3. If CopyAllOn is set, copy the array.
  //    If CopyAllOff is set, do not copy the array
  virtual void CopyAllOff(int ctype=ALLCOPY);

  // -- passthrough operations ----------------------------------------------

  // Description:
  // Pass entire arrays of input data through to output. Obey the "copy"
  // flags. When passing a field,  the following copying rules are
  // followed: 1) Check if a field is an attribute, if yes and if there
  // is a PASSDATA copy flag for that attribute (on or off), obey the flag
  // for that attribute, ignore (2) and (3), 2) if there is a copy field for
  // that field (on or off), obey the flag, ignore (3) 3) obey
  // CopyAllOn/Off
  virtual void PassData(vtkFieldData* fd);

  // -- copytuple operations ------------------------------------------------

  // Description:
  // Allocates point data for point-by-point (or cell-by-cell) copy operation.
  // If sze=0, then use the input DataSetAttributes to create (i.e., find
  // initial size of) new objects; otherwise use the sze variable.
  // Note that pd HAS to be the vtkDataSetAttributes object which
  // will later be used with CopyData. If this is not the case,
  // consider using the alternative forms of CopyAllocate and CopyData.
  // ext is no longer used.
  // If shallowCopyArrays is true, input arrays are copied to the output
  // instead of new ones being allocated.
  void CopyAllocate(vtkDataSetAttributes* pd, vtkIdType sze=0,
                    vtkIdType ext=1000)
    {
      this->CopyAllocate(pd, sze, ext, 0);
    }
  void CopyAllocate(vtkDataSetAttributes* pd, vtkIdType sze,
                    vtkIdType ext, int shallowCopyArrays);

  // Description:
  // This method is used to copy data arrays in images.
  // You should call "CopyAllocate" before calling this method.
  void CopyStructuredData(vtkDataSetAttributes *inDsa,
                          const int *inExt, const int *outExt);

  // Description:
  // Copy the attribute data from one id to another. Make sure CopyAllocate()
  // has been invoked before using this method. When copying a field,
  // the following copying rules are
  // followed: 1) Check if a field is an attribute, if yes and if there
  // is a COPYTUPLE copy flag for that attribute (on or off), obey the  flag
  // for that attribute, ignore (2) and (3), 2) if there is a copy field for
  // that field (on or off), obey the flag, ignore (3) 3) obey
  // CopyAllOn/Off
  void CopyData(vtkDataSetAttributes *fromPd, vtkIdType fromId, vtkIdType toId);


  // Description:
  // Copy a tuple of data from one data array to another. This method
  // assumes that the fromData and toData objects are of the
  // same type, and have the same number of components. This is true if you
  // invoke CopyAllocate() or InterpolateAllocate().
  void CopyTuple(vtkAbstractArray *fromData, vtkAbstractArray *toData,
                 vtkIdType fromId, vtkIdType toId);


  // -- interpolate operations ----------------------------------------------

  // Description:
  // Initialize point interpolation method.
  // Note that pd HAS to be the vtkDataSetAttributes object which
  // will later be used with InterpolatePoint or InterpolateEdge.
  // ext is no longer used.
  // If shallowCopyArrays is true, input arrays are copied to the output
  // instead of new ones being allocated.
  void InterpolateAllocate(vtkDataSetAttributes* pd, vtkIdType sze=0,
                           vtkIdType ext=1000)
    {
      this->InterpolateAllocate(pd, sze, ext, 0);
    }
  void InterpolateAllocate(vtkDataSetAttributes* pd, vtkIdType sze,
                           vtkIdType ext, int shallowCopyArrays);

  // Description:
  // Interpolate data set attributes from other data set attributes
  // given cell or point ids and associated interpolation weights.
  // If the INTERPOLATION copy flag is set to 0 for an array, interpolation
  // is prevented. If the flag is set to 1, weighted interpolation occurs.
  // If the flag is set to 2, nearest neighbor interpolation is used.
  void InterpolatePoint(vtkDataSetAttributes *fromPd, vtkIdType toId,
                        vtkIdList *ids, double *weights);

  // Description:
  // Interpolate data from the two points p1,p2 (forming an edge) and an
  // interpolation factor, t, along the edge. The weight ranges from (0,1),
  // with t=0 located at p1. Make sure that the method InterpolateAllocate()
  // has been invoked before using this method.
  // If the INTERPOLATION copy flag is set to 0 for an array, interpolation
  // is prevented. If the flag is set to 1, weighted interpolation occurs.
  // If the flag is set to 2, nearest neighbor interpolation is used.
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
  // If the INTERPOLATION copy flag is set to 0 for an array, interpolation
  // is prevented. If the flag is set to 1, weighted interpolation occurs.
  // If the flag is set to 2, nearest neighbor interpolation is used.
  void InterpolateTime(vtkDataSetAttributes *from1,
                       vtkDataSetAttributes *from2,
                       vtkIdType id, double t);

//BTX
  class FieldList;

  // field list copy operations ------------------------------------------

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

  // Description:
  // A special form of InterpolateAllocate() to be used with FieldLists. Use it
  // when you are interpolating data from a set of vtkDataSetAttributes.
  // \c Warning: This does not copy the Information object associated with
  // each data array. This behavior may change in the future.
  void InterpolateAllocate(vtkDataSetAttributes::FieldList& list, vtkIdType sze=0,
                    vtkIdType ext=1000);

  // Description:
  // Interpolate data set attributes from other data set attributes
  // given cell or point ids and associated interpolation weights.
  // Make sure that special form of InterpolateAllocate() that accepts
  // FieldList has been used.
  void InterpolatePoint(
    vtkDataSetAttributes::FieldList& list,
    vtkDataSetAttributes *fromPd,
    int idx, vtkIdType toId,
    vtkIdList *ids, double *weights);

  friend class vtkDataSetAttributes::FieldList;
//ETX

//BTX
protected:
  vtkDataSetAttributes();
  ~vtkDataSetAttributes();

  void InternalCopyAllocate(vtkDataSetAttributes* pd,
                            int ctype,
                            vtkIdType sze=0,
                            vtkIdType ext=1000,
                            int shallowCopyArrays=0);

  void InternalCopyAllocate(
    vtkDataSetAttributes::FieldList& list,
    int ctype,
    vtkIdType sze, vtkIdType ext);

  // Description:
  // Initialize all of the object's data to NULL
  virtual void InitializeFields();

  int AttributeIndices[NUM_ATTRIBUTES]; //index to attribute array in field data
  int CopyAttributeFlags[ALLCOPY][NUM_ATTRIBUTES]; //copy flag for attribute data

  vtkFieldData::BasicIterator RequiredArrays;

  int* TargetIndices;

  static const int NumberOfAttributeComponents[NUM_ATTRIBUTES];
  static const int AttributeLimits[NUM_ATTRIBUTES];
  static const char AttributeNames[NUM_ATTRIBUTES][12];
  static const char LongAttributeNames[NUM_ATTRIBUTES][35];

private:
  int SetAttribute(vtkAbstractArray* da, int attributeType);
  static int CheckNumberOfComponents(vtkAbstractArray* da, int attributeType);

  vtkFieldData::BasicIterator  ComputeRequiredArrays(vtkDataSetAttributes* pd, int ctype);

private:
  vtkDataSetAttributes(const vtkDataSetAttributes&);  // Not implemented.
  void operator=(const vtkDataSetAttributes&);  // Not implemented.

public:
  // This public class is used to perform set operations, other misc.
  // operations on fields. For example, vtkAppendFilter uses it to
  // determine which attributes the input datasets share in common.
  class vtkInternalComponentNames;
  class VTKCOMMONDATAMODEL_EXPORT FieldList
  {
  public:
    FieldList(int numInputs);
    ~FieldList();
    void PrintSelf(ostream &os, vtkIndent indent);

    void InitializeFieldList(vtkDataSetAttributes* dsa);
    void IntersectFieldList(vtkDataSetAttributes* dsa);

    // Description:
    // Similar to IntersectFieldList() except that it builds a union of the
    // array list. To determine the active attributes, it still, however, takes
    // an intersection.
    // WARNING!!!-IntersectFieldList() and UnionFieldList() should not be
    // intermixed.
    void UnionFieldList(vtkDataSetAttributes* dsa);

    //Determine whether data is available
    int IsAttributePresent(int attrType); //true/false attributes specified

    // Accessor methods.
    int GetNumberOfFields() { return this->NumberOfFields; }
    int GetFieldIndex(int i) { return this->FieldIndices[i]; }
    const char* GetFieldName(int i) { return this->Fields[i]; }
    int GetFieldComponents(int i) { return this->FieldComponents[i]; }
    int GetDSAIndex(int index, int i) { return this->DSAIndices[index][i]; }

    friend class vtkDataSetAttributes;

  protected:
    FieldList(const FieldList&) {} //prevent these methods from being used
    void operator=(const FieldList&) {}

    void SetFieldIndex(int i, int index)
      { this->FieldIndices[i] = index; }
  private:
    void SetField(int index, vtkAbstractArray *da);
    void RemoveField(const char *name);
    void ClearFields();
    void GrowBy(unsigned int delta);

    int NumberOfFields; //the number of fields (including five named attributes)
    // These keep track of what is common across datasets. The first
    // six items are always named attributes.
    char** Fields;                     // the names of the fields
    int *FieldTypes;                   // the types of the fields
    int *FieldComponents;              // the number of components in field
    int *FieldIndices;                 // output data array index
    vtkLookupTable **LUT;              // luts associated with each array
    vtkInformation **FieldInformation; // Information map associated with each array

    vtkInternalComponentNames **FieldComponentsNames;       // the name for each component in the field

    vtkIdType NumberOfTuples; // a running total of values

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


