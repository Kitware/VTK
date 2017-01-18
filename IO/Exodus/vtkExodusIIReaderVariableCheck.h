#ifndef vtkExodusIIReaderVariableCheck_h
#define vtkExodusIIReaderVariableCheck_h

#include "vtkExodusIIReaderPrivate.h" // for ArrayInfoType

#include <vtksys/RegularExpression.hxx> // for integration point names
#include <string> // STL Header for Start/StartInternal/Add
#include <vector> // STL Header for glommed array names
#include <set> // STL Header for integration point names

/**\brief Abstract base class for glomming arrays of variable names.
  *
  * Subclasses check whether variable names listed in an array of names
  * are related to each other (and should thus be glommed into a single
  * VTK array).
  */
class vtkExodusIIReaderVariableCheck
{
public:
  /// Initialize a sequence of names. Returns true if any more names are acceptable.
  virtual bool Start( std::string name, const int* truth, int numTruth );
  /// Subclasses implement this and returns true if any more names are acceptable.
  virtual bool StartInternal( std::string name, const int* truth, int numTruth ) = 0;
  /// Add a name to the sequence. Returns true if any more names may be added.
  virtual bool Add( std::string name, const int* truth ) = 0;
  /// Returns the length of the sequence (or 0 if the match is incorrect or incomplete).
  virtual std::vector<std::string>::size_type Length();
  /// Accept this sequence. (Add an entry to the end of \a arr.) Must return Length().
  virtual int Accept(
    std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arr,
    int startIndex, vtkExodusIIReaderPrivate* priv, int objtyp );

protected:
  vtkExodusIIReaderVariableCheck();
  virtual ~vtkExodusIIReaderVariableCheck();
  /** Utility that subclasses may call from within Add() to verify that
    * the new variable is defined on the same objects as other variables in the sequence.
    */
  bool CheckTruth( const int* truth );
  bool UniquifyName(
    vtkExodusIIReaderPrivate::ArrayInfoType& ainfo,
    std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arrays );

  int GlomType;
  std::vector<int> SeqTruth;
  std::string Prefix;
  std::vector<std::string> OriginalNames;
};

/// This always accepts a single array name as a scalar. It is the fallback for all other checkers.
class vtkExodusIIReaderScalarCheck : public vtkExodusIIReaderVariableCheck
{
public:
  vtkExodusIIReaderScalarCheck();
  bool StartInternal( std::string name, const int*, int ) VTK_OVERRIDE;
  bool Add( std::string, const int* ) VTK_OVERRIDE;
};

/// This looks for n-D vectors whose names are identical except for a single final character.
class vtkExodusIIReaderVectorCheck : public vtkExodusIIReaderVariableCheck
{
public:
  vtkExodusIIReaderVectorCheck( const char* seq, int n );
  bool StartInternal( std::string name, const int*, int ) VTK_OVERRIDE;
  bool Add( std::string name, const int* truth ) VTK_OVERRIDE;
  std::vector<std::string>::size_type Length() VTK_OVERRIDE;
protected:
  std::string Endings;
  bool StillAdding;
};

/**\brief This looks for symmetric tensors of a given rank and dimension.
  *
  * All array names must be identical except for the last \a rank characters
  * which must be taken from the \a dim -length character array \a seq, specified
  * as dimension indicators.
  */
class vtkExodusIIReaderTensorCheck : public vtkExodusIIReaderVariableCheck
{
public:
  vtkExodusIIReaderTensorCheck( const char* seq, int n, int rank, int dim );
  bool StartInternal( std::string name, const int*, int ) VTK_OVERRIDE;
  bool Add( std::string name, const int* truth ) VTK_OVERRIDE;
  std::vector<std::string>::size_type Length() VTK_OVERRIDE;
protected:
  std::string Endings;
  vtkTypeUInt64 NumEndings;
  int Dimension;
  int Rank;
  bool StillAdding;
};

/// This looks for integration-point variables whose names contain an element shape and digits specifying an integration point.
class vtkExodusIIReaderIntPointCheck : public vtkExodusIIReaderVariableCheck
{
public:
  vtkExodusIIReaderIntPointCheck();
  bool StartInternal( std::string name, const int*, int ) VTK_OVERRIDE;
  bool Add( std::string name, const int* ) VTK_OVERRIDE;
  std::vector<std::string>::size_type Length() VTK_OVERRIDE;
  /*
  virtual int Accept(
    std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arr, int startIndex, vtkExodusIIReaderPrivate* priv, int objtyp )
    {
    }
    */
protected:
  bool StartIntegrationPoints( std::string cellType, std::string iptName );
  bool AddIntegrationPoint( std::string iptName );

  vtksys::RegularExpression RegExp;
  std::string VarName;
  std::string CellType;
  std::vector<int> IntPtMin;
  std::vector<int> IntPtMax;
  std::set<std::string> IntPtNames;
  vtkTypeUInt64 Rank;
  bool StillAdding;
};

#endif // vtkExodusIIReaderVariableCheck_h
// VTK-HeaderTest-Exclude: vtkExodusIIReaderVariableCheck.h
