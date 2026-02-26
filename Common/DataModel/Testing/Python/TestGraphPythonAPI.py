"""Tests for the Pythonic graph/tree API."""

from vtkmodules.vtkCommonDataModel import (
    vtkMutableDirectedGraph,
    vtkMutableUndirectedGraph,
    vtkTree,
)
from vtkmodules.util.graph import Edge
from vtkmodules.test import Testing


class TestGraphPythonAPI(Testing.vtkTest):

    def _build_directed_graph(self):
        """Build a directed graph: 0->1->2, 0->3->4."""
        g = vtkMutableDirectedGraph()
        for _ in range(5):
            g.AddVertex()
        g.AddEdge(0, 1)
        g.AddEdge(1, 2)
        g.AddEdge(0, 3)
        g.AddEdge(3, 4)
        return g

    def _build_undirected_graph(self):
        """Build an undirected graph: 0-1-2, 0-3-4."""
        g = vtkMutableUndirectedGraph()
        for _ in range(5):
            g.AddVertex()
        g.AddEdge(0, 1)
        g.AddEdge(1, 2)
        g.AddEdge(0, 3)
        g.AddEdge(3, 4)
        return g

    def _build_tree(self):
        """Build a tree from directed graph: 0->1->2, 0->3->4."""
        g = vtkMutableDirectedGraph()
        for _ in range(5):
            g.AddVertex()
        g.AddEdge(0, 1)
        g.AddEdge(1, 2)
        g.AddEdge(0, 3)
        g.AddEdge(3, 4)
        t = vtkTree()
        t.CheckedShallowCopy(g)
        return t

    # -- len, iter, vertices --

    def test_len(self):
        g = self._build_directed_graph()
        self.assertEqual(len(g), 5)

    def test_iter(self):
        g = self._build_directed_graph()
        self.assertEqual(list(g), [0, 1, 2, 3, 4])

    def test_vertices(self):
        g = self._build_directed_graph()
        verts = g.vertices
        self.assertEqual(len(verts), 5)
        self.assertEqual(list(verts), [0, 1, 2, 3, 4])
        # range is subscriptable
        self.assertEqual(verts[2], 2)

    # -- edges --

    def test_edges_directed(self):
        g = self._build_directed_graph()
        edges = list(g.edges)
        self.assertEqual(len(edges), 4)
        # Check namedtuple fields
        self.assertIsInstance(edges[0], Edge)
        self.assertEqual(edges[0].source, 0)
        self.assertEqual(edges[0].target, 1)
        self.assertEqual(edges[0].id, 0)

    def test_edges_unpacking(self):
        g = self._build_directed_graph()
        for s, t, eid in g.edges:
            self.assertIsInstance(s, int)
            self.assertIsInstance(t, int)
            self.assertIsInstance(eid, int)

    # -- neighbors --

    def test_neighbors_directed(self):
        g = self._build_directed_graph()
        nbrs = sorted(g.neighbors(0))
        self.assertEqual(nbrs, [1, 3])

    def test_neighbors_leaf(self):
        g = self._build_directed_graph()
        nbrs = list(g.neighbors(2))
        self.assertEqual(nbrs, [])

    # -- out_edges, in_edges --

    def test_out_edges(self):
        g = self._build_directed_graph()
        out = list(g.out_edges(0))
        self.assertEqual(len(out), 2)
        targets = sorted(e.target for e in out)
        self.assertEqual(targets, [1, 3])

    def test_in_edges(self):
        g = self._build_directed_graph()
        inc = list(g.in_edges(1))
        self.assertEqual(len(inc), 1)
        self.assertEqual(inc[0].source, 0)
        self.assertEqual(inc[0].target, 1)

    # -- degree --

    def test_degree(self):
        g = self._build_directed_graph()
        self.assertEqual(g.degree(0), 2)
        self.assertEqual(g.degree(1), 2)  # 1 in + 1 out
        self.assertEqual(g.degree(2), 1)

    def test_out_degree(self):
        g = self._build_directed_graph()
        self.assertEqual(g.out_degree(0), 2)
        self.assertEqual(g.out_degree(2), 0)

    def test_in_degree(self):
        g = self._build_directed_graph()
        self.assertEqual(g.in_degree(0), 0)
        self.assertEqual(g.in_degree(1), 1)

    # -- vertex_data, edge_data --

    def test_vertex_data(self):
        g = self._build_directed_graph()
        vd = g.vertex_data
        self.assertIsNotNone(vd)
        self.assertEqual(vd.GetClassName(), "vtkDataSetAttributes")

    def test_edge_data(self):
        g = self._build_directed_graph()
        ed = g.edge_data
        self.assertIsNotNone(ed)
        self.assertEqual(ed.GetClassName(), "vtkDataSetAttributes")

    # -- repr --

    def test_repr_directed(self):
        g = self._build_directed_graph()
        r = repr(g)
        self.assertIn("5 vertices", r)
        self.assertIn("4 edges", r)

    # -- undirected graph --

    def test_undirected_len(self):
        g = self._build_undirected_graph()
        self.assertEqual(len(g), 5)

    def test_undirected_edges(self):
        g = self._build_undirected_graph()
        edges = list(g.edges)
        self.assertEqual(len(edges), 4)

    def test_undirected_neighbors(self):
        g = self._build_undirected_graph()
        nbrs = sorted(g.neighbors(1))
        self.assertEqual(nbrs, [0, 2])

    def test_undirected_in_out_edges_equal(self):
        """For undirected graphs, in_edges and out_edges should be the same."""
        g = self._build_undirected_graph()
        out = sorted(g.out_edges(1), key=lambda e: e.target)
        inc = sorted(g.in_edges(1), key=lambda e: e.source)
        self.assertEqual(len(out), len(inc))

    # -- empty graph --

    def test_empty_graph(self):
        g = vtkMutableDirectedGraph()
        self.assertEqual(len(g), 0)
        self.assertEqual(list(g), [])
        self.assertEqual(list(g.edges), [])

    # -- tree --

    def test_tree_root(self):
        t = self._build_tree()
        self.assertEqual(t.root, 0)

    def test_tree_parent(self):
        t = self._build_tree()
        self.assertEqual(t.parent(1), 0)
        self.assertEqual(t.parent(2), 1)
        self.assertEqual(t.parent(3), 0)

    def test_tree_children(self):
        t = self._build_tree()
        children = sorted(t.children(0))
        self.assertEqual(children, [1, 3])
        children_of_1 = list(t.children(1))
        self.assertEqual(children_of_1, [2])

    def test_tree_is_leaf(self):
        t = self._build_tree()
        self.assertFalse(t.is_leaf(0))
        self.assertFalse(t.is_leaf(1))
        self.assertTrue(t.is_leaf(2))
        self.assertTrue(t.is_leaf(4))

    def test_tree_level(self):
        t = self._build_tree()
        self.assertEqual(t.level(0), 0)
        self.assertEqual(t.level(1), 1)
        self.assertEqual(t.level(2), 2)

    def test_tree_repr(self):
        t = self._build_tree()
        r = repr(t)
        self.assertIn("5 vertices", r)
        self.assertIn("root=0", r)

    def test_tree_inherits_graph_methods(self):
        """Tree should have all graph methods via _TreeMixin(_GraphMixin)."""
        t = self._build_tree()
        self.assertEqual(len(t), 5)
        self.assertEqual(list(t), [0, 1, 2, 3, 4])
        edges = list(t.edges)
        self.assertEqual(len(edges), 4)
        self.assertIsNotNone(t.vertex_data)
        self.assertIsNotNone(t.edge_data)


if __name__ == "__main__":
    Testing.main([(TestGraphPythonAPI, "test")])
