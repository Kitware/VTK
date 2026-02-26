"""Pythonic read-only API for VTK graph and tree classes.

Adds iteration, len, repr, and convenient accessors::

    g = vtkMutableDirectedGraph()
    # ... build graph ...
    len(g)                          # vertex count
    for v in g:                     # iterate vertex ids
    for s, t, eid in g.edges:       # iterate edges as namedtuples
    for n in g.neighbors(5):        # adjacent vertex ids
    g.vertex_data                   # vtkDataSetAttributes
    g.edge_data                     # vtkDataSetAttributes

    # Tree-specific:
    t.root                          # root vertex id
    for c in t.children(v):         # child vertex ids
    t.parent(v)                     # parent vertex id
    t.is_leaf(v)                    # bool
"""

from collections import namedtuple

from vtkmodules.vtkCommonDataModel import (
    vtkDirectedAcyclicGraph,
    vtkDirectedGraph,
    vtkMutableDirectedGraph,
    vtkMutableUndirectedGraph,
    vtkTree,
    vtkUndirectedGraph,
)

Edge = namedtuple("Edge", ["source", "target", "id"])


class _GraphMixin:
    def __len__(self):
        return self.GetNumberOfVertices()

    def __iter__(self):
        return iter(range(self.GetNumberOfVertices()))

    def __repr__(self):
        return "%s(%d vertices, %d edges)" % (
            self.GetClassName(),
            self.GetNumberOfVertices(),
            self.GetNumberOfEdges(),
        )

    @property
    def vertices(self):
        return range(self.GetNumberOfVertices())

    @property
    def edges(self):
        from vtkmodules.vtkCommonDataModel import vtkEdgeListIterator

        it = vtkEdgeListIterator()
        self.GetEdges(it)
        while it.HasNext():
            e = it.NextGraphEdge()
            yield Edge(e.GetSource(), e.GetTarget(), e.GetId())

    @property
    def vertex_data(self):
        return self.GetVertexData()

    @property
    def edge_data(self):
        return self.GetEdgeData()

    def neighbors(self, v):
        from vtkmodules.vtkCommonDataModel import vtkAdjacentVertexIterator

        it = vtkAdjacentVertexIterator()
        self.GetAdjacentVertices(v, it)
        while it.HasNext():
            yield it.Next()

    def out_edges(self, v):
        from vtkmodules.vtkCommonDataModel import vtkOutEdgeIterator

        it = vtkOutEdgeIterator()
        self.GetOutEdges(v, it)
        while it.HasNext():
            e = it.NextGraphEdge()
            yield Edge(e.GetSource(), e.GetTarget(), e.GetId())

    def in_edges(self, v):
        from vtkmodules.vtkCommonDataModel import vtkInEdgeIterator

        it = vtkInEdgeIterator()
        self.GetInEdges(v, it)
        while it.HasNext():
            e = it.NextGraphEdge()
            yield Edge(e.GetSource(), e.GetTarget(), e.GetId())

    def degree(self, v):
        return self.GetDegree(v)

    def out_degree(self, v):
        return self.GetOutDegree(v)

    def in_degree(self, v):
        return self.GetInDegree(v)


class _TreeMixin(_GraphMixin):
    @property
    def root(self):
        return self.GetRoot()

    def parent(self, v):
        return self.GetParent(v)

    def children(self, v):
        for i in range(self.GetNumberOfChildren(v)):
            yield self.GetChild(v, i)

    def is_leaf(self, v):
        return self.IsLeaf(v)

    def level(self, v):
        return self.GetLevel(v)

    def __repr__(self):
        return "%s(%d vertices, root=%d)" % (
            self.GetClassName(),
            self.GetNumberOfVertices(),
            self.GetRoot(),
        )


@vtkDirectedGraph.override
class DirectedGraph(_GraphMixin, vtkDirectedGraph):
    pass


@vtkMutableDirectedGraph.override
class MutableDirectedGraph(_GraphMixin, vtkMutableDirectedGraph):
    pass


@vtkUndirectedGraph.override
class UndirectedGraph(_GraphMixin, vtkUndirectedGraph):
    pass


@vtkMutableUndirectedGraph.override
class MutableUndirectedGraph(_GraphMixin, vtkMutableUndirectedGraph):
    pass


@vtkDirectedAcyclicGraph.override
class DirectedAcyclicGraph(_GraphMixin, vtkDirectedAcyclicGraph):
    pass


@vtkTree.override
class Tree(_TreeMixin, vtkTree):
    pass
