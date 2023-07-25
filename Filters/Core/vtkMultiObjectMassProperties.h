// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiObjectMassProperties
 * @brief   compute volume and area of objects in a polygonal mesh
 *
 * vtkMultiObjectMassProperties estimates the volume, the surface area, and
 * the centroid of a polygonal mesh. Multiple, valid closed objects may be
 * represented, and each object is assumed to be defined as a polyhedron
 * defined by polygonal faces (i.e., the faces do not have to be triangles).
 * The algorithm computes the total volume and area, as well as per object values
 * which are placed in data arrays. Note that an object is valid only if it is
 * manifold and closed (i.e., each edge is used exactly two times by two different
 * polygons). Invalid objects are processed but may produce inaccurate
 * results. Inconsistent polygon ordering is also allowed.
 *
 * The algorithm is composed of two basic parts. First a connected traversal
 * is performed to identify objects, detect whether the objects are valid,
 * and ensure that the composing polygons are ordered consistently. Next, in
 * threaded execution, a parallel process of computing areas, volumes  and
 * centroids is performed. It is possible to skip the first part if the
 * SkipValidityCheck is enabled, AND a vtkIdTypeArray data array named "ObjectIds"
 * is associated with the polygon input (i.e., cell data) that enumerates which
 * object every polygon belongs to (i.e., indicates that it is a boundary
 * polygon of a specified object).
 *
 * The algorithm implemented here is inspired by this paper:
 * http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf. Also see
 * the stack-overflow entry: https://stackoverflow.com/questions/1406029/.
 * The centroids are calculated as a weighted average of the centroids of the tetrahedrons
 * which are used to compute the volume of the polygonal object, and the weight is the
 * tetrahedron's volume contribution. If the polygonal object has 0 volume, then the resulted
 * centroid will be (nan, nan, nan). The general assumption here is that the model
 * is of closed surface.  Also, this approach requires triangulating the polygons so
 * triangle meshes are processed much faster. Finally, the volume, area and centroid
 * calculations are done in parallel (threaded) after a connectivity pass is made
 * (used to identify objects and verify that they are manifold and closed).
 *
 * The output contains seven additional data arrays. The arrays
 * "ObjectValidity", "ObjectVolumes", "ObjectAreas" and "ObjectCentroids" are
 * placed in the output field data.  These are arrays which indicate which objects are
 * valid; the volume of each object; the surface area of each object; the centroid
 * of each object. Three additional arrays are placed in the output cell data, and
 * indicate, on a per polygons basis, which object the polygon bounds
 * "ObjectIds"; the polygon area "Areas"; and the contribution of volume
 * "Volumes".  Additionally, the TotalVolume and TotalArea is available after
 * the filter executes (i.e., the sum of the ObjectVolumes and ObjectAreas
 * arrays).
 *
 * Per-object validity, as mentioned previously, is reported in the
 * ObjectValidity array. However another variable, AllValid, is set after
 * filter execution which indicates whether all objects are valid (!=0) or
 * not. This information can be used as a shortcut in case you want to skip
 * validity checking on an object-by-object basis.
 *
 * @warning
 * This filter operates on the polygonal data contained in the input
 * vtkPolyData. Other types (vertices, lines, triangle strips) are ignored and
 * not passed to the output. The input polys and points, as well as
 * associated point and cell data, are passed through to the output.
 *
 * @warning
 * This filter is similar to vtkMassProperties. However vtkMassProperties
 * operates on triangle meshes and assumes only a single, closed, properly
 * oriented surface is represented. vtkMultiObjectMassProperties performs
 * additional topological and connectivity operations to identify separate
 * objects, and confirms that they are manifold. It also accommodates
 * inconsistent ordering.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkMassProperties
 */

#ifndef vtkMultiObjectMassProperties_h
#define vtkMultiObjectMassProperties_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;

class VTKFILTERSCORE_EXPORT vtkMultiObjectMassProperties : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type and printing.
   */
  static vtkMultiObjectMassProperties* New();
  vtkTypeMacro(vtkMultiObjectMassProperties, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Indicate whether to skip the validity check (the first part of the
   * algorithm). By default this is off; however even if enabled validity
   * skipping will only occur if a vtkIdTypeArray named "ObjectIds" is also
   * provided on input to the filter.
   */
  vtkSetMacro(SkipValidityCheck, vtkTypeBool);
  vtkGetMacro(SkipValidityCheck, vtkTypeBool);
  vtkBooleanMacro(SkipValidityCheck, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the ObjectIds array. This array, which indicates in which object
   * a polygon belongs to, can be either provided by the user or computed.
   * The default name is "ObjectIds".
   */
  vtkSetStdStringFromCharMacro(ObjectIdsArrayName);
  vtkGetCharFromStdStringMacro(ObjectIdsArrayName);
  ///@}

  /**
   * Return the number of objects identified. This is valid only after the
   * filter executes. Check the ObjectValidity array which indicates which of
   * these identified objects are valid. Invalid objects may have incorrect
   * volume and area values.
   */
  vtkIdType GetNumberOfObjects() { return this->NumberOfObjects; }

  /**
   * Return whether all objects are valid or not. This is valid only after the
   * filter executes.
   */
  vtkTypeBool GetAllValid() { return this->AllValid; }

  /**
   * Return the summed volume of all objects. This is valid only after the
   * filter executes.
   */
  double GetTotalVolume() { return this->TotalVolume; }

  /**
   * Return the summed area of all objects. This is valid only after the
   * filter executes.
   */
  double GetTotalArea() { return this->TotalArea; }

protected:
  vtkMultiObjectMassProperties();
  ~vtkMultiObjectMassProperties() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Data members supporting API
  vtkTypeBool SkipValidityCheck;
  vtkTypeBool AllValid;
  double TotalVolume;
  double TotalArea;

  // Internal data members supporting algorithm execution
  vtkIdType NumberOfObjects;      // number of objects identified
  vtkIdTypeArray* ObjectIds;      // for each input polygon, the object id that the polygon is in
  std::string ObjectIdsArrayName; // the array name of ObjectIds.

  vtkUnsignedCharArray* ObjectValidity; // is it a valid object?
  vtkDoubleArray* ObjectVolumes;        // what is the object volume (if valid)?
  vtkDoubleArray* ObjectAreas;          // what is the total object area?
  vtkDoubleArray* ObjectCentroids;      // what is the object centroid

  vtkIdList* CellNeighbors; // avoid repetitive new/delete
  vtkIdList* Wave;          // processing wave
  vtkIdList* Wave2;

  // Connected traversal to identify objects
  void TraverseAndMark(
    vtkPolyData* output, vtkIdType* objectIds, vtkDataArray* valid, unsigned char* orient);

private:
  vtkMultiObjectMassProperties(const vtkMultiObjectMassProperties&) = delete;
  void operator=(const vtkMultiObjectMassProperties&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
