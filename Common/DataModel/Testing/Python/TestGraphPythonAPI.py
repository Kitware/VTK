"""Tests for the Pythonic graph/tree API."""

import numpy as np

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

    # -- bulk edge setter: directed --

    def test_set_edges_directed(self):
        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2], [0, 3], [3, 4]])
        self.assertEqual(len(g), 5)
        self.assertEqual(g.GetNumberOfEdges(), 4)
        edges = sorted(g.edges, key=lambda e: e.id)
        self.assertEqual(edges, [
            Edge(0, 1, 0), Edge(1, 2, 1), Edge(0, 3, 2), Edge(3, 4, 3)
        ])

    def test_set_edges_directed_roundtrip(self):
        """Set edges, read them back, set again — should produce same result."""
        g = vtkMutableDirectedGraph()
        arr = np.array([[0, 1], [1, 2], [2, 0]])
        g.edges = arr
        edges1 = list(g.edges)
        # Set again to verify reset works
        g.edges = arr
        edges2 = list(g.edges)
        self.assertEqual(edges1, edges2)

    def test_set_edges_directed_vertex_count(self):
        """Vertex count should be max(vertex_id) + 1."""
        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 5]])
        self.assertEqual(len(g), 6)

    # -- bulk edge setter: undirected --

    def test_set_edges_undirected(self):
        g = vtkMutableUndirectedGraph()
        g.edges = np.array([[0, 1], [1, 2], [0, 3]])
        self.assertEqual(len(g), 4)
        self.assertEqual(g.GetNumberOfEdges(), 3)
        edges = list(g.edges)
        self.assertEqual(len(edges), 3)

    def test_set_edges_undirected_neighbors(self):
        """Undirected edges should be traversable in both directions."""
        g = vtkMutableUndirectedGraph()
        g.edges = np.array([[0, 1], [1, 2]])
        self.assertEqual(sorted(g.neighbors(1)), [0, 2])

    def test_set_edges_undirected_self_loop(self):
        """Self-loops should not double-count in undirected graphs."""
        g = vtkMutableUndirectedGraph()
        g.edges = np.array([[0, 1], [1, 1]])
        self.assertEqual(g.GetNumberOfEdges(), 2)
        # vertex 1: edge to/from 0 (1 OutEdge entry) + self-loop (1 OutEdge entry) = degree 2
        self.assertEqual(g.degree(1), 2)

    # -- vertex_data / edge_data setters --

    def test_vertex_data_setter(self):
        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2]])
        g.vertex_data = {"weight": np.array([1.0, 2.0, 3.0])}
        vd = g.vertex_data
        self.assertEqual(vd.GetNumberOfArrays(), 1)
        self.assertEqual(vd.GetArray("weight").GetValue(0), 1.0)
        self.assertEqual(vd.GetArray("weight").GetValue(2), 3.0)

    def test_edge_data_setter(self):
        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2]])
        g.edge_data = {"cost": np.array([0.5, 1.5])}
        ed = g.edge_data
        self.assertEqual(ed.GetNumberOfArrays(), 1)
        self.assertEqual(ed.GetArray("cost").GetValue(0), 0.5)
        self.assertEqual(ed.GetArray("cost").GetValue(1), 1.5)

    def test_vertex_data_setter_replaces(self):
        """Setting vertex_data should replace existing arrays."""
        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1]])
        g.vertex_data = {"a": np.array([1.0, 2.0])}
        g.vertex_data = {"b": np.array([3.0, 4.0])}
        vd = g.vertex_data
        self.assertEqual(vd.GetNumberOfArrays(), 1)
        self.assertIsNone(vd.GetArray("a"))
        self.assertIsNotNone(vd.GetArray("b"))


    # -- NetworkX interop --

    def _skip_without_networkx(self):
        try:
            import networkx  # noqa: F401
        except ImportError:
            self.skipTest("networkx not installed")

    def test_to_networkx_directed(self):
        """Directed VTK graph exports as nx.DiGraph."""
        self._skip_without_networkx()
        import networkx as nx

        g = self._build_directed_graph()
        G = g.to_networkx()
        self.assertIsInstance(G, nx.DiGraph)
        self.assertEqual(G.number_of_nodes(), 5)
        self.assertEqual(G.number_of_edges(), 4)
        self.assertTrue(G.has_edge(0, 1))
        self.assertTrue(G.has_edge(1, 2))
        self.assertTrue(G.has_edge(0, 3))
        self.assertTrue(G.has_edge(3, 4))

    def test_to_networkx_undirected(self):
        """Undirected VTK graph exports as nx.Graph."""
        self._skip_without_networkx()
        import networkx as nx

        g = self._build_undirected_graph()
        G = g.to_networkx()
        self.assertIsInstance(G, nx.Graph)
        self.assertNotIsInstance(G, nx.DiGraph)
        self.assertEqual(G.number_of_nodes(), 5)
        self.assertEqual(G.number_of_edges(), 4)

    def test_to_networkx_vertex_data(self):
        """Vertex data arrays are exported as node attributes."""
        self._skip_without_networkx()

        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2]])
        g.vertex_data = {"weight": np.array([1.0, 2.0, 3.0])}
        G = g.to_networkx()
        self.assertEqual(G.nodes[0]["weight"], 1.0)
        self.assertEqual(G.nodes[1]["weight"], 2.0)
        self.assertEqual(G.nodes[2]["weight"], 3.0)

    def test_to_networkx_edge_data(self):
        """Edge data arrays are exported as edge attributes."""
        self._skip_without_networkx()

        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2]])
        g.edge_data = {"cost": np.array([0.5, 1.5])}
        G = g.to_networkx()
        self.assertEqual(G.edges[0, 1]["cost"], 0.5)
        self.assertEqual(G.edges[1, 2]["cost"], 1.5)

    def test_to_networkx_empty(self):
        """Empty graph converts to empty networkx graph."""
        self._skip_without_networkx()

        g = vtkMutableDirectedGraph()
        G = g.to_networkx()
        self.assertEqual(G.number_of_nodes(), 0)
        self.assertEqual(G.number_of_edges(), 0)

    def test_directed_roundtrip(self):
        """VTK directed -> nx.DiGraph -> VTK directed preserves structure."""
        self._skip_without_networkx()

        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1], [1, 2], [0, 3]])
        g.vertex_data = {"weight": np.array([1.0, 2.0, 3.0, 4.0])}
        g.edge_data = {"cost": np.array([0.5, 1.5, 2.5])}

        G = g.to_networkx()

        g2 = vtkMutableDirectedGraph()
        g2.from_networkx(G)

        self.assertEqual(len(g2), 4)
        self.assertEqual(g2.GetNumberOfEdges(), 3)

        edges2 = sorted((s, t) for s, t, _ in g2.edges)
        self.assertEqual(edges2, [(0, 1), (0, 3), (1, 2)])

        vd = g2.vertex_data
        w = vd.GetArray("weight")
        self.assertAlmostEqual(w.GetValue(0), 1.0)
        self.assertAlmostEqual(w.GetValue(3), 4.0)

        ed = g2.edge_data
        c = ed.GetArray("cost")
        self.assertIsNotNone(c)
        self.assertEqual(c.GetNumberOfTuples(), 3)

    def test_undirected_roundtrip(self):
        """VTK undirected -> nx.Graph -> VTK undirected preserves structure."""
        self._skip_without_networkx()

        g = vtkMutableUndirectedGraph()
        g.edges = np.array([[0, 1], [1, 2], [0, 3]])

        G = g.to_networkx()

        g2 = vtkMutableUndirectedGraph()
        g2.from_networkx(G)

        self.assertEqual(len(g2), 4)
        self.assertEqual(g2.GetNumberOfEdges(), 3)

    def test_from_networkx_non_integer_nodes(self):
        """from_networkx raises ValueError for non-contiguous integer nodes."""
        self._skip_without_networkx()
        import networkx as nx

        G = nx.Graph()
        G.add_edges_from([("a", "b"), ("b", "c")])

        g = vtkMutableUndirectedGraph()
        with self.assertRaises((ValueError, TypeError)):
            g.from_networkx(G)

    def test_from_networkx_non_contiguous_nodes(self):
        """from_networkx raises ValueError for non-contiguous integer nodes."""
        self._skip_without_networkx()
        import networkx as nx

        G = nx.Graph()
        G.add_nodes_from([0, 2, 5])
        G.add_edge(0, 2)

        g = vtkMutableUndirectedGraph()
        with self.assertRaises(ValueError):
            g.from_networkx(G)

    def test_from_networkx_empty(self):
        """from_networkx with empty graph produces empty VTK graph."""
        self._skip_without_networkx()
        import networkx as nx

        G = nx.DiGraph()
        g = vtkMutableDirectedGraph()
        g.from_networkx(G)
        self.assertEqual(len(g), 0)
        self.assertEqual(g.GetNumberOfEdges(), 0)

    def test_roundtrip_multicomponent_vertex_data(self):
        """Multi-component vertex data survives roundtrip as tuples."""
        self._skip_without_networkx()

        g = vtkMutableDirectedGraph()
        g.edges = np.array([[0, 1]])
        g.vertex_data = {"pos": np.array([[1.0, 2.0], [3.0, 4.0]])}

        G = g.to_networkx()
        self.assertEqual(G.nodes[0]["pos"], (1.0, 2.0))
        self.assertEqual(G.nodes[1]["pos"], (3.0, 4.0))


if __name__ == "__main__":
    Testing.main([(TestGraphPythonAPI, "test")])
