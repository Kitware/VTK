/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// This class was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef MESH_H
#define MESH_H

#include <vector>
class Node;
class Cell;

class vtkDataSet;

class Mesh
{
 public:
  // constructeurs / destructeur
  Mesh (int xnode, int ynode, int znode, Node* n1, Node* n2, Node* n3, Node* n4, Node* n5, Node* n6, Node* n7, Node* n8);
  ~Mesh ();

  // raffine de 1 niveau
  void refine();
  // nombre de decoupage a chaque raffinement
  void setFactor (int factor);

  // suppression des points identiques
  void mergePoints ();

  // creation d'une grille de mailles
  std::vector<Cell*> & createCells (int xnode, int ynode, int znode, 
                                    Node* n1, Node* n2, Node* n3, Node* n4,
                                    Node* n5, Node* n6, Node* n7, Node* n8, 
                                    Cell * fromCell = 0);

  // creation d'un dataset VTK
  vtkDataSet * getDataSet();

  static Mesh * instance() {return _instance;}

 protected:
  void addCell (Cell * c);
  void addNode (Node * n);

  int getNextNodeId();
  int getNextCellId();
      
  std::vector<Node*> _nodes;
  std::vector<Cell*> _cells;
      
  std::vector<Cell*> _lastCreatedCells;

  int _lastCellId;
  int _lastNodeId;
  int _branchFactor;

  vtkDataSet * _dataSet;

 private:
  static Mesh * _instance;
} ;

#endif /* MESH_H */
