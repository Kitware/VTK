"""Pythonic API for VTK graph and tree classes.

Adds iteration, len, repr, and convenient accessors::

    g = vtkMutableDirectedGraph()
    # ... build graph ...
    len(g)                          # vertex count
    for v in g:                     # iterate vertex ids
    for s, t, eid in g.edges:       # iterate edges as namedtuples
    for n in g.neighbors(5):        # adjacent vertex ids
    g.vertex_data                   # vtkDataSetAttributes
    g.edge_data                     # vtkDataSetAttributes

    # Mutable graph construction:
    g = vtkMutableDirectedGraph()
    g.edges = np.array([[0, 1], [1, 2], [0, 3]])
    g.vertex_data = {"weight": np.array([1.0, 2.0, 3.0, 4.0])}
    g.edge_data = {"cost": np.array([0.5, 1.5, 2.5])}

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


class _MutableGraphMixin(_GraphMixin):
    @property
    def edges(self):
        from vtkmodules.vtkCommonDataModel import vtkEdgeListIterator

        it = vtkEdgeListIterator()
        self.GetEdges(it)
        while it.HasNext():
            e = it.NextGraphEdge()
            yield Edge(e.GetSource(), e.GetTarget(), e.GetId())

    @edges.setter
    def edges(self, edge_array):
        """Set all edges from an (E, 2) array of [source, target].

        Accepts a numpy array or any vtkDataArray with 2 components.
        The number of vertices is inferred from the maximum vertex id.
        Any existing edges and vertices are cleared.
        """
        import numpy

        from vtkmodules.util.numpy_support import numpy_to_vtk

        if isinstance(edge_array, numpy.ndarray):
            edge_array = numpy.ascontiguousarray(edge_array)
            vtk_arr = numpy_to_vtk(edge_array)
            vtk_arr.SetNumberOfComponents(2)
            vtk_arr.SetNumberOfTuples(len(edge_array))
        else:
            vtk_arr = edge_array
        self.SetEdges(vtk_arr)

    @property
    def vertex_data(self):
        return self.GetVertexData()

    @vertex_data.setter
    def vertex_data(self, arrays):
        """Set vertex arrays from a dict of ``{name: array}``.

        Each value can be a numpy array or a VTK data array.
        Existing vertex arrays are removed first.
        """
        vd = self.GetVertexData()
        vd.Initialize()
        _set_arrays(vd, arrays, self.GetNumberOfVertices())

    @property
    def edge_data(self):
        return self.GetEdgeData()

    @edge_data.setter
    def edge_data(self, arrays):
        """Set edge arrays from a dict of ``{name: array}``.

        Each value can be a numpy array or a VTK data array.
        Existing edge arrays are removed first.
        """
        ed = self.GetEdgeData()
        ed.Initialize()
        _set_arrays(ed, arrays, self.GetNumberOfEdges())


def _set_arrays(dsa, arrays, num_tuples):
    """Populate a vtkDataSetAttributes from a dict of {name: array}."""
    import numpy

    from vtkmodules.util.numpy_support import numpy_to_vtk
    from vtkmodules.vtkCommonCore import vtkAbstractArray

    dsa.SetNumberOfTuples(num_tuples)
    for name, arr in arrays.items():
        if isinstance(arr, numpy.ndarray):
            vtk_arr = numpy_to_vtk(arr)
        elif isinstance(arr, vtkAbstractArray):
            vtk_arr = arr
        else:
            vtk_arr = numpy_to_vtk(numpy.asarray(arr))
        vtk_arr.SetName(name)
        dsa.AddArray(vtk_arr)


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
class MutableDirectedGraph(_MutableGraphMixin, vtkMutableDirectedGraph):
    pass


@vtkUndirectedGraph.override
class UndirectedGraph(_GraphMixin, vtkUndirectedGraph):
    pass


@vtkMutableUndirectedGraph.override
class MutableUndirectedGraph(_MutableGraphMixin, vtkMutableUndirectedGraph):
    pass


@vtkDirectedAcyclicGraph.override
class DirectedAcyclicGraph(_GraphMixin, vtkDirectedAcyclicGraph):
    pass


@vtkTree.override
class Tree(_TreeMixin, vtkTree):
    pass
