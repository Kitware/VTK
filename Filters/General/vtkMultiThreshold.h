/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkMultiThreshold - Threshold cells within multiple intervals
// .SECTION Description
// This filter can be substituted for a chain of several vtkThreshold filters
// and can also perform more sophisticated subsetting operations.
// It generates a vtkMultiBlockDataSet as its output.
// This multiblock dataset contains a vtkUnstructuredGrid for each thresholded
// subset you request.
// A thresholded subset can be a set defined by an interval over a
// point or cell attribute of the mesh; these subsets are called IntervalSets.
// A thresholded subset can also be a boolean combination of one or more IntervalSets;
// these subsets are called BooleanSets.
// BooleanSets allow complex logic since their output
// can depend on multiple intervals over multiple variables
// defined on the input mesh.
// This is useful because it eliminates the need for thresholding several
// times and then appending the results, as can be required with vtkThreshold
// when one wants to remove some range of values (e.g., a notch filter).
// Cells are not repeated when they belong to more than one interval unless
// those intervals have different output grids.
//
// Another advantage this filter provides over vtkThreshold is the ability
// to threshold on non-scalar (i.e., vector, tensor, etc.) attributes without
// first computing an array containing some norm of the desired attribute.
// vtkMultiThreshold provides \f$L_1\f$, \f$L_2\f$, and \f$L_{\infty}\f$ norms.
//
// This filter makes a distinction between intermediate subsets and
// subsets that will be output to a grid.
// Each intermediate subset you create with AddIntervalSet or
// AddBooleanSet is given a unique integer identifier (via the return
// values of these member functions).
// If you wish for a given set to be output, you must call
// OutputSet and pass it one of these identifiers.
// The return of OutputSet is the integer index of the output set
// in the multiblock dataset created by this filter.
//
// For example, if an input mesh defined three attributes T, P, and s, one might
// wish to find cells that satisfy "T < 320 [K] && ( P > 101 [kPa] || s < 0.1 [kJ/kg/K] )".
// To accomplish this with a vtkMultiThreshold filter,
// <pre>
// vtkMultiThreshold* thr;
// int intervalSets[3];
//
// intervalSets[0] = thr->AddIntervalSet( vtkMath::NegInf(), 320., vtkMultiThreshold::CLOSED, vtkMultiThreshold::OPEN,
//     vtkDataObject::FIELD_ASSOCIATION_POINTS, "T", 0, 1 );
// intervalSets[1] = thr->AddIntervalSet( 101., vtkMath::Inf(), vtkMultiThreshold::OPEN, vtkMultiThreshold::CLOSED,
//     vtkDataObject::FIELD_ASSOCIATION_CELLS, "P", 0, 1 );
// intervalSets[2] = thr->AddIntervalSet( vtkMath::NegInf(), 0.1, vtkMultiThreshold::CLOSED, vtkMultiThreshold::OPEN,
//     vtkDataObject::FIELD_ASSOCIATION_POINTS, "s", 0, 1 );
//
// int intermediate = thr->AddBooleanSet( vtkMultiThreshold::OR, 2, &intervalSets[1] );
//
// int intersection[2];
// intersection[0] = intervalSets[0];
// intersection[1] = intermediate;
// int outputSet = thr->AddBooleanSet( vtkMultiThreshold::AND, 2, intersection );
//
// int outputGridIndex = thr->OutputSet( outputSet );
// thr->Update();
// </pre>
// The result of this filter will be a multiblock dataset that contains a single child with the desired cells.
// If we had also called <code>thr->OutputSet( intervalSets[0] );</code>, there would be two child meshes and
// one would contain all cells with T < 320 [K].
// In that case, the output can be represented by this graph
// \dot
// digraph MultiThreshold {
//   set0 [shape=rect,style=filled,label="point T(0) in [-Inf,320["]
//   set1 [shape=rect,label="cell P(0) in ]101,Inf]"]
//   set2 [shape=rect,label="point s(0) in [-Inf,0.1["]
//   set3 [shape=rect,label="OR"]
//   set4 [shape=rect,style=filled,label="AND"]
//   set0 -> set4
//   set1 -> set3
//   set2 -> set3
//   set3 -> set4
// }
// \enddot
// The filled rectangles represent sets that are output.

#ifndef __vtkMultiThreshold_h
#define __vtkMultiThreshold_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkMath.h" // for Inf() and NegInf()

#include <vector> // for lists of threshold rules
#include <map> // for IntervalRules map
#include <set> // for UpdateDependents()
#include <string> // for holding array names in NormKey

class vtkCell;
class vtkCellData;
class vtkDataArray;
class vtkGenericCell;
class vtkPointSet;
class vtkUnstructuredGrid;

class VTKFILTERSGENERAL_EXPORT vtkMultiThreshold : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMultiThreshold,vtkMultiBlockDataSetAlgorithm);
  static vtkMultiThreshold* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  //BTX
  /// Whether the endpoint value of an interval should be included or excluded.
  enum Closure {
    OPEN=0,   //!< Specify an open interval
    CLOSED=1  //!< Specify a closed interval
  };
  /// Norms that can be used to threshold vector attributes.
  enum Norm {
    LINFINITY_NORM=-3, //!< Use the \f$L_{\infty}\f$ norm for the specified array threshold.
    L2_NORM=-2,        //!< Use the \f$L_2\f$ norm for the specified array threshold.
    L1_NORM=-1         //!< Use the \f$L_1\f$ norm for the specified array threshold.
  };
  /// Operations that can be performed on sets to generate another set. Most of these operators take 2 or more input sets.
  enum SetOperation {
    AND, //!< Only include an element if it belongs to all the input sets
    OR, //!< Include an element if it belongs to any input set
    XOR, //!< Include an element if it belongs to exactly one input set
    WOR, //!< Include elements that belong to an odd number of input sets (a kind of "winding XOR")
    NAND //!< Only include elements that don't belong to any input set
  };
  //ETX

  // Description:
  // Add a mesh subset to be computed by thresholding an attribute of the input mesh.
  // The subset can then be added to an output mesh with OuputSet() or combined with other sets using AddBooleanSet.
  // If you wish to include all cells with values below some number \a a, call
  // with xmin set to vtkMath::NegInf() and xmax set to \a a.
  // Similarly, if you wish to include all cells with values above some number \a a,
  // call with xmin set to \a a and xmax set to vtkMath::Inf().
  // When specifying Inf() or NegInf() for an endpoint, it does not matter whether
  // you specify and open or closed endpoint.
  //
  // When creating intervals, any integers can be used for the IDs of output meshes.
  // All that matters is that the same ID be used if intervals should output to the same mesh.
  // The outputs are ordered with ascending IDs in output block 0.
  //
  // It is possible to specify an invalid interval, in which case these routines will return -1.
  // Invalid intervals occur when
  // - an array does not exist,
  // - \a center is invalid,
  // - \a xmin == \a xmax and \a omin and/or \a omax are vtkMultiThreshold::OPEN, or
  // - \a xmin > \a xmax.
  // - \a xmin or \a xmax is not a number (i.e., IEEE NaN). Having both \a xmin and \a xmax equal NaN is allowed.
  // vtkMath provides a portable way to specify IEEE infinities and Nan.
  // Note that specifying an interval completely out of the bounds of an attribute is considered valid.
  // In fact, it is occasionally useful to create a closed interval with both endpoints set to \f$\infty\f$
  // or both endpoints set to \f$-\infty\f$ in order to locate cells with problematic values.
  //
  // @param xmin The minimum attribute value
  // @param xmax The maximum attribute value
  // @param omin Whether the interval should be open or closed at \a xmin. Use vtkMultiThreshold::OPEN or vtkMultiThreshold::CLOSED.
  // @param omax Whether the interval should be open or closed at \a xmax. Use vtkMultiThreshold::OPEN or vtkMultiThreshold::CLOSED.
  // @param assoc One of vtkDataObject::FIELD_ASSOCIATION_CELLS or vtkDataObject::FIELD_ASSOCIATION_POINTS indicating whether
  //               a point or cell array should be used.
  // @param arrayName The name of the array to use for thresholding
  // @param attribType The attribute to use for thresholding.
  //                   One of vtkDataSetAttributes::SCALARS, VECTORS, TENSORS, NORMALS, TCOORDS, or GLOBALIDS.
  // @param component The number of the component to threshold on or one of the following enumerants for norms:
  //                  LINFINITY_NORM, L2_NORM, L1_NORM.
  // @param allScalars When \a center is vtkDataObject::FIELD_ASSOCIATION_POINTS, must all scalars be in the interval for
  //                   the cell to be passed to the output, or just a single point's scalar?
  // @return An index used to identify the cells selected by the interval or -1 if the interval specification was invalid.
  //         If a valid value is returned, you may pass it to OutputSet().
  int AddIntervalSet( double xmin, double xmax, int omin, int omax,
    int assoc, const char* arrayName, int component, int allScalars );
  int AddIntervalSet( double xmin, double xmax, int omin, int omax,
    int assoc, int attribType, int component, int allScalars );

  // Description:
  // These convenience members make it easy to insert closed intervals.
  // The "notch" interval is accomplished by creating a bandpass interval and applying a NAND operation.
  // In this case, the set ID returned in the NAND operation set ID.
  // Note that you can pass xmin == xmax when creating a bandpass threshold to retrieve elements matching exactly
  // one value (since the intervals created by these routines are closed).
  int AddLowpassIntervalSet( double xmax, int assoc, const char* arrayName, int component, int allScalars );
  int AddHighpassIntervalSet( double xmin, int assoc, const char* arrayName, int component, int allScalars );
  int AddBandpassIntervalSet( double xmin, double xmax, int assoc, const char* arrayName, int component, int allScalars );
  int AddNotchIntervalSet( double xlo, double xhi, int assoc, const char* arrayName, int component, int allScalars );

  // Description:
  // Create a new mesh subset using boolean operations on pre-existing sets.
  int AddBooleanSet( int operation, int numInputs, int* inputs );

  // Description:
  // Create an output mesh containing a boolean or interval subset of the input mesh.
  int OutputSet( int setId );

  // Description:
  // Remove all the intervals currently defined.
  void Reset();

  //BTX
  /// A pointer to a function that returns a norm (or a single component) of a tuple with 1 or more components.
  typedef double (*TupleNorm)( vtkDataArray* arr, vtkIdType tuple, int component );

  // NormKey must be able to use TupleNorm typedef:
  class NormKey;

  // Interval must be able to use NormKey typedef:
  class Interval;

  // Set needs to refer to boolean set pointers
  class BooleanSet;

  /// A class with comparison operator used to index input array norms used in threshold rules.
  class NormKey {
  public:
    int Association; // vtkDataObject::FIELD_ASSOCIATION_POINTS or vtkDataObject::FIELD_ASSOCIATION_CELLS
    int Type; // -1 => use Name, otherwise: vtkDataSetAttributes::{SCALARS, VECTORS, TENSORS, NORMALS, TCOORDS, GLOBALIDS}
    std::string Name; // Either empty or (when ArrayType == -1) an input array name
    int Component; // LINFINITY_NORM, L1_NORM, L2_NORM or an integer component number
    int AllScalars; // For Association == vtkDataObject::FIELD_ASSOCIATION_POINTS, must all points be in the interval?
    int InputArrayIndex; // The number passed to vtkAlgorithm::SetInputArrayToProcess()
    TupleNorm NormFunction; // A function pointer to compute the norm (or fetcht the correct component) of a tuple.

    /// Compute the norm of a cell by calling NormFunction for all its points or for its single cell-centered value.
    void ComputeNorm( vtkIdType cellId, vtkCell* cell, vtkDataArray* array, double cellNorm[2] ) const;

    /// A partial ordering of NormKey objects is required for them to serve as keys in the vtkMultiThreshold::IntervalRules map.
    bool operator < ( const NormKey& other ) const {
      if ( this->Association < other.Association )
        return true;
      else if ( this->Association > other.Association )
        return false;

      if ( this->Component < other.Component )
        return true;
      else if ( this->Component > other.Component )
        return false;

      if ( (! this->AllScalars) && other.AllScalars )
        return true;
      else if ( this->AllScalars && (! other.AllScalars) )
        return false;

      if ( this->Type == -1 )
        {
        if ( other.Type == -1 )
          return this->Name < other.Name;
        return true;
        }
      else
        {
        return this->Type < other.Type;
        }
    }
  };

  /** A base class for representing threshold sets.
   * A set may be represented as a threshold interval over some attribute
   * or as a boolean combination of sets.
   */
  class Set {
  public:
    int Id; /// A unique identifier for this set.
    int OutputId; /// The index of the output mesh that will hold this set or -1 if the set is not output.

    /// Default constructur. The grid output ID is initialized to indicate that the set should not be output.
    Set() {
      this->OutputId = -1;
    }
    /// Virtual destructor since we have virtual members.
    virtual ~Set() { }
    /// Print a graphviz node label statement (with fancy node name and shape).
    virtual void PrintNodeName( ostream& os );
    /// Print a graphviz node name for use in an edge statement.
    virtual void PrintNode( ostream& os ) = 0;
    /// Avoid dynamic_casts. Subclasses must override
    virtual BooleanSet* GetBooleanSetPointer();
    virtual Interval* GetIntervalPointer();
  };

  /// A subset of a mesh represented by a range of acceptable attribute values.
  class Interval : public Set {
  public:
    /// The values defining the interval. These must be in ascending order.
    double EndpointValues[2];
    /// Are the endpoint values themselves included in the set (CLOSED) or not (OPEN)?
    int EndpointClosures[2];
    /// This contains information about the attribute over which the interval is defined.
    NormKey Norm;

    /** Does the specified range fall inside the interval?
     * For cell-centered attributes, only cellNorm[0] is examined.
     * For point-centered attributes, cellNorm[0] is the minimum norm taken on over the cell and cellNorm[1] is the maximum.
     */
    int Match( double cellNorm[2] );

    virtual ~Interval() { }
    virtual void PrintNode( ostream& os );
    virtual Interval* GetIntervalPointer();
  };

  /// A subset of a mesh represented as a boolean set operation
  class BooleanSet : public Set {
  public:
    /// The boolean operation that will be performed on the inputs to obtain the output.
    int Operator;
    /// A list of input sets. These may be IntervalSets or BooleanSets.
    std::vector<int> Inputs;

    /// Construct a new set with the given ID, operator, and inputs.
    BooleanSet( int sId, int op, int* inBegin, int* inEnd ) : Inputs( inBegin, inEnd ) {
      this->Id = sId;
      this->Operator = op;
    }
    virtual ~BooleanSet() { }
    virtual void PrintNode( ostream& os );
    virtual BooleanSet* GetBooleanSetPointer();
  };
  //ETX

protected:

  vtkMultiThreshold();
  virtual ~vtkMultiThreshold();

  //BTX
  // Description:
  // When an interval is evaluated, its value is used to update a truth table.
  // If its value allows the output of the truth table to be determined, then
  // either INCLUDE or EXCLUDE is returned. Otherwise, INCONCLUSIVE is returned
  // and more intervals must be evaluated.
  //
  // As an example, consider the ruleset {A>10} & ( {6<B<8} | {C==12} ).
  // We first evaluate A. Say A is 100. This makes the first rule true, but the
  // value of the rule *set* is still indeterminate. INCONCLUSIVE is returned.
  // Next we consider B. If B is 7, then INCLUDE will be returned and there is
  // no need to examine C.  If B is 0, then INCONCLUSIVE is returned again and
  // we must examine C. If C is 12, then INCLUDE is returned, otherwise EXCLUDE
  // is returned.
  enum Ruling {
    INCONCLUSIVE=-1,
    INCLUDE=-2,
    EXCLUDE=-3
  };
  //ETX

  // Description:
  // This function performs the actual thresholding.
  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  // Description:
  // We accept any mesh that is descended from vtkPointSet.
  // In the future, it is possible to accept more types but this would require
  // us to generate a new vtkPoints object for each output mesh.
  virtual int FillInputPortInformation( int port, vtkInformation* info );

  // Description:
  // A variable used to store the next index to use when calling SetInputArrayToProcess.
  // Its value is stored in an interval's ArrayIndex member and used during RequestData
  // to retrieve a pointer to the actual array.
  int NextArrayIndex;

  // Description:
  // The number of output datasets.
  int NumberOfOutputs;

  //BTX
  /// A list of pointers to IntervalSets.
  typedef std::vector<Interval*> IntervalList;
  /// A map describing the IntervalSets that share a common attribute and norm.
  typedef std::map<NormKey,IntervalList> RuleMap;

  typedef std::vector<int> TruthTreeValues;
  typedef std::vector<TruthTreeValues> TruthTree;

  // Description:
  // A set of threshold rules sorted by the attribute+norm to which they are applied.
  RuleMap IntervalRules;

  // Description:
  // A list of rules keyed by their unique integer ID.
  // This list is used to quickly determine whether interval membership implies membership in a given output mesh.
  std::vector<Set*> Sets;

  // Description:
  // A list of boolean sets whose values depend on the given set.
  // Each time an interval is evaluated for a cell, the list of dependent boolean sets
  // contained here is updated. Any boolean operations whose truth values are decided
  // are then marked and <i>their</i> dependent sets are examined.
  TruthTree DependentSets;

  // Description:
  // Recursively update the setStates and unresolvedOutputs vectors based on this->DependentSets.
  void UpdateDependents(
    int id, std::set<int>& unresolvedOutputs, TruthTreeValues& setStates,
    vtkCellData* inCellData, vtkIdType cellId, vtkGenericCell* cell, std::vector<vtkUnstructuredGrid*>& outv );

  // Description:
  // A utility method called by the public AddInterval members.
  int AddIntervalSet( NormKey& nk, double xmin, double xmax, int omin, int omax );

  //ETX

  // Description:
  // Print out a graphviz-formatted text description of all the sets.
  void PrintGraph( ostream& os );

  vtkMultiThreshold( const vtkMultiThreshold& ); // Not implemented.
  void operator = ( const vtkMultiThreshold& ); // Not implemented.
};

inline int vtkMultiThreshold::AddLowpassIntervalSet( double xmax, int assoc, const char* arrayName, int component, int allScalars )
{
  return this->AddIntervalSet( vtkMath::NegInf(), xmax, CLOSED, CLOSED, assoc, arrayName, component, allScalars );
}

inline int vtkMultiThreshold::AddHighpassIntervalSet( double xmin, int assoc, const char* arrayName, int component, int allScalars )
{
  return this->AddIntervalSet( xmin, vtkMath::Inf(), CLOSED, CLOSED, assoc, arrayName, component, allScalars );
}

inline int vtkMultiThreshold::AddBandpassIntervalSet(
  double xmin, double xmax, int assoc, const char* arrayName, int component, int allScalars )
{
  return this->AddIntervalSet( xmin, xmax, CLOSED, CLOSED, assoc, arrayName, component, allScalars );
}

inline int vtkMultiThreshold::AddNotchIntervalSet(
  double xlo, double xhi, int assoc, const char* arrayName, int component, int allScalars )
{
  int band = this->AddIntervalSet( xlo, xhi, CLOSED, CLOSED, assoc, arrayName, component, allScalars );
  if ( band < 0 )
    {
    return -1;
    }
  return this->AddBooleanSet( NAND, 1, &band );
}

inline vtkMultiThreshold::Interval* vtkMultiThreshold::Set::GetIntervalPointer()
{
  return 0;
}

inline vtkMultiThreshold::BooleanSet* vtkMultiThreshold::Set::GetBooleanSetPointer()
{
  return 0;
}

inline vtkMultiThreshold::Interval* vtkMultiThreshold::Interval::GetIntervalPointer()
{
  return this;
}

inline vtkMultiThreshold::BooleanSet* vtkMultiThreshold::BooleanSet::GetBooleanSetPointer()
{
  return this;
}

#endif // __vtkMultiThreshold_h
