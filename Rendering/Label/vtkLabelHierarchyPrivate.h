#ifndef __vtkLabelHierarchyPrivate_h
#define __vtkLabelHierarchyPrivate_h

#include "vtkObject.h" // for vtkstd
#include <set>

#include "octree/octree"

//----------------------------------------------------------------------------
// vtkLabelHierarchy::Implementation

class vtkLabelHierarchy::Implementation
{
public:
  Implementation()
    {
    this->Hierarchy2 = 0;
    this->Hierarchy3 = 0;
    this->ActualDepth = 5;
    this->Z2 = 0.;
    }

  ~Implementation()
    {
    if ( this->Hierarchy2 )
      {
      delete this->Hierarchy2;
      }
    if ( this->Hierarchy3 )
      {
      delete this->Hierarchy3;
      }

    }

  bool ComparePriorities( vtkIdType a, vtkIdType b )
    {
    vtkDataArray* priorities = this->Husk->GetPriorities();
    return priorities ?
      priorities->GetTuple1( a ) > priorities->GetTuple1( b ) :
      a < b;
    }

  struct PriorityComparator
  {
    vtkLabelHierarchy* Hierarchy;

    PriorityComparator()
      {
      // See comment near declaration of Current for more info:
      this->Hierarchy = vtkLabelHierarchy::Implementation::Current;
      }

    PriorityComparator( vtkLabelHierarchy* h )
      {
      this->Hierarchy = h;
      }

    PriorityComparator( const PriorityComparator& src )
      {
      this->Hierarchy = src.Hierarchy;
      }

    PriorityComparator& operator=(const PriorityComparator& rhs)
      {
      if (this != &rhs)
        {
        this->Hierarchy = rhs.Hierarchy;
        }
      return *this;
      }

    ~PriorityComparator()
      {
      }

    bool operator () ( const vtkIdType& a, const vtkIdType& b )
      {
      if (0 == this->Hierarchy)
        {
        vtkGenericWarningMacro( "error: NULL this->Hierarchy in PriorityComparator" );
        return a < b;
        }

      if (0 == this->Hierarchy->GetImplementation())
        {
        vtkGenericWarningMacro( "error: NULL this->Hierarchy->GetImplementation() in PriorityComparator" );
        return a < b;
        }

      return this->Hierarchy->GetImplementation()->ComparePriorities( a, b );
      }
  };

  class LabelSet : public std::multiset<vtkIdType,PriorityComparator>
  {
  public:
    LabelSet( vtkLabelHierarchy* hierarchy )
      : std::multiset<vtkIdType,PriorityComparator>( PriorityComparator(hierarchy) )
      {
      this->TotalAnchors = 0;
      this->Size = 1.;
      for ( int i = 0; i < 3; ++ i )
        {
        this->Center[i] = 0.;
        }
      }

    LabelSet( const LabelSet& src )
      : std::multiset<vtkIdType,PriorityComparator>( src )
      {
      this->TotalAnchors = src.TotalAnchors;
      this->Size = src.Size;
      for ( int i = 0; i < 3; ++ i )
        {
        this->Center[i] = src.Center[i];
        }
      }

    LabelSet()
      : std::multiset<vtkIdType,PriorityComparator>()
      {
      this->TotalAnchors = 0;
      this->Size = 1.;
      for ( int i = 0; i < 3; ++ i )
        {
        this->Center[i] = 0.;
        }
      }

    LabelSet& operator = ( const LabelSet& rhs )
      {
      if ( this != &rhs )
        {
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
        std::multiset<vtkIdType,PriorityComparator>::operator = ( rhs );
#endif
        this->TotalAnchors = rhs.TotalAnchors;
        this->Size = rhs.Size;
        for ( int i = 0; i < 3; ++ i )
          {
          this->Center[i] = rhs.Center[i];
          }
        }
      return *this;
      }
    const double* GetCenter() const { return this->Center; }
    double GetSize() const { return this->Size; }
    void SetGeometry( const double center[3], double length );
    void SetChildGeometry( octree<LabelSet,2>::octree_node_pointer self );
    void SetChildGeometry( octree<LabelSet,3>::octree_node_pointer self );
    void AddChildren( octree<LabelSet,2>::octree_node_pointer self, LabelSet& emptyNode );
    void AddChildren( octree<LabelSet,3>::octree_node_pointer self, LabelSet& emptyNode );
    void Insert( vtkIdType anchor )
      {
      this->insert( anchor );
      ++ this->TotalAnchors;
      }
    void Increment() { ++ this->TotalAnchors; }
    vtkIdType GetLocalAnchorCount() const { return this->size(); }
    vtkIdType GetTotalAnchorCount() const { return this->TotalAnchors; }

    vtkIdType TotalAnchors; // Count of all anchors stored in this node and its children.
    double Center[3]; // Geometric coordinates of this node's center.
    double Size; // Length of each edge of this node.
  };

  typedef octree<LabelSet,2> HierarchyType2;
  typedef octree<LabelSet,2>::cursor HierarchyCursor2;
  typedef octree<LabelSet,2>::iterator HierarchyIterator2;

  typedef octree<LabelSet> HierarchyType3;
  typedef octree<LabelSet>::cursor HierarchyCursor3;
  typedef octree<LabelSet>::iterator HierarchyIterator3;

  //typedef std::map<Coord,std::pair<int,std::set<vtkIdType> > >::iterator MapCoordIter;

  // Description:
  // Computes the depth of the generated hierarchy.
  //void ComputeActualDepth();

  // Description:
  // Routines called by ComputeHierarchy()
  void BinAnchorsToLevel( int level );
  void PromoteAnchors();
  void DemoteAnchors( int level );
  void RecursiveNodeDivide( HierarchyCursor2& cursor );
  void RecursiveNodeDivide( HierarchyCursor3& cursor );

  // Description:
  // Routines called by ComputeHierarchy()
  void PrepareSortedAnchors( LabelSet& anchors );
  void FillHierarchyRoot( LabelSet& anchors );
  void DropAnchor2( vtkIdType anchor );
  void DropAnchor3( vtkIdType anchor );
  void SmudgeAnchor2( HierarchyCursor2& cursor, vtkIdType anchor, double* x );
  void SmudgeAnchor3( HierarchyCursor3& cursor, vtkIdType anchor, double* x );

  double Z2; // common z-coordinate of all label anchors when quadtree (Hierarchy2) is used.
  HierarchyType2* Hierarchy2; // 2-D quadtree of label anchors (all input points have same z coord)
  HierarchyType3* Hierarchy3; // 3-D octree of label anchors (input point bounds have non-zero z range)
  vtkTimeStamp HierarchyTime;
  HierarchyType3::size_type ActualDepth;
  vtkLabelHierarchy* Husk;

  static vtkLabelHierarchy* Current;
};

inline void vtkLabelHierarchy::Implementation::LabelSet::SetGeometry( const double center[3], double length )
{
  for ( int i = 0; i < 3; ++ i )
    {
    this->Center[i] = center[i];
    }
  this->Size = length;
}

inline void vtkLabelHierarchy::Implementation::LabelSet::SetChildGeometry( octree<LabelSet,2>::octree_node_pointer self )
{
  double sz2 = this->Size / 2.;
  double x[3];
  for ( int i = 0; i < self->num_children(); ++ i )
    {
    for ( int j = 0; j < 2; ++ j )
      {
      x[j] = this->Center[j] + ( ( i & (1<<j) ) ? 0.5 : -0.5 ) * sz2 ;
      }
    x[2] = this->Center[2];
    (*self)[i].value().SetGeometry( x, sz2 );
    }
}

inline void vtkLabelHierarchy::Implementation::LabelSet::SetChildGeometry( octree<LabelSet,3>::octree_node_pointer self )
{
  double sz2 = this->Size / 2.;
  double x[3];
  for ( int i = 0; i < self->num_children(); ++ i )
    {
    for ( int j = 0; j < 3; ++ j )
      {
      x[j] = this->Center[j] + ( ( i & (1<<j) ) ? 0.5 : -0.5 ) * sz2 ;
      }
    (*self)[i].value().SetGeometry( x, sz2 );
    }
}

inline void vtkLabelHierarchy::Implementation::LabelSet::AddChildren( octree<LabelSet,2>::octree_node_pointer self, LabelSet& emptyNode )
{
  self->add_children( emptyNode );
  this->SetChildGeometry( self );
}

inline void vtkLabelHierarchy::Implementation::LabelSet::AddChildren( octree<LabelSet,3>::octree_node_pointer self, LabelSet& emptyNode )
{
  self->add_children( emptyNode );
  this->SetChildGeometry( self );
}

#endif // __vtkLabelHierarchyPrivate_h
// VTK-HeaderTest-Exclude: vtkLabelHierarchyPrivate.h
