/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// This class was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef CELL_H
#define CELL_H

#include <vector>

#include <vtkType.h>

class Node;

class Cell
{
   public:
      Cell (int id, std::vector<Node*> nodes);
      ~Cell ();
      void replaceNode (Node * oldNode, Node * newNode);
      bool isRefined () {return _refined;}
      int getId () {return _id;}
      vtkIdType * getNodeIds();
      void setNeighbours(int idx1, int idx2, int idy1, int idy2, int idz1, int idz2) {}

      void refine ();
      void refineIfNeeded();
      static void setRefine (int refine) {_refineNumber = refine;}
      static void setR (double R) {_R = R;}
      
      static int getCount() {return _count;}
      
   protected:
      std::vector<Node*> _nodes;
      std::vector<Cell*> _cells;
      bool _refined;
      int _id;
      vtkIdType * _nodeIds;

   private:
      double computeValue (Node * n);
      static int _count;
      static int _refinedCount;
      static int _refineNumber;
      static double _R;
} ;

#endif /* CELL_H */
