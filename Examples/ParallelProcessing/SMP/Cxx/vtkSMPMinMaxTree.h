#ifndef VTKSMPMINMAXTREE_H
#define VTKSMPMINMAXTREE_H

#include "vtkSimpleScalarTree.h"
#include "vtkDataSet.h"

class vtkGenericCell;

class vtkScalarNode {};

template <class TScalar>
class vtkScalarRange : public vtkScalarNode
{
public:
  TScalar min;
  TScalar max;
};

class VTK_EXPORT vtkSMPMinMaxTree : public vtkSimpleScalarTree
{
  vtkSMPMinMaxTree( const vtkSMPMinMaxTree& );
  void operator =( const vtkSMPMinMaxTree& );

protected:
  vtkSMPMinMaxTree();
  ~vtkSMPMinMaxTree();

  vtkIdType CutOff;

public:
  vtkTypeMacro(vtkSMPMinMaxTree, vtkSimpleScalarTree);
  static vtkSMPMinMaxTree* New();
  void PrintSelf(ostream &os, vtkIndent indent);

  void BuildTree();
  void InitTraversal(double scalarValue);

  template<typename Functor>
  int TraverseNode( vtkIdType id, int lvl, Functor& function ) const
  {
  if ( id >= this->TreeSize )
    {
    return 0;
    }

  vtkScalarRange<double> *t = static_cast<vtkScalarRange<double>*>(this->Tree) + id;
  if ( t->min > this->ScalarValue || t->max < this->ScalarValue )
    {
    return 0;
    }

  if ( lvl <= this->Level - this->CutOff ) //leaf
    {
    vtkIdType cell_id = ( id - this->LeafOffset ) * this->BranchingFactor;
    vtkIdType max_id = this->DataSet->GetNumberOfCells();
    vtkIdType end_id = cell_id + this->BranchingFactor;
    function(cell_id, end_id > max_id ? max_id : end_id);
    return 0;
    }
  else //node
    {
    return 1;
    }
  }
  virtual void GetTreeSize ( int& max_level, vtkIdType& branching_factor ) const;

};

#endif // VTKSMPMINMAXTREE_H
