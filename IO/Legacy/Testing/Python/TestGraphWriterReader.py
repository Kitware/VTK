from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkIOLegacy as il

from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir

import os

class TestGraphWriterReader(Testing.vtkTest):

    def test(self):

        mdgTree = dm.vtkMutableDirectedGraph()
        mug = dm.vtkMutableUndirectedGraph()

        for i in range(10):
            mdgTree.AddVertex()
            mug.AddVertex()

        mdgTree.AddEdge(0, 1)
        mdgTree.AddEdge(0, 2)
        mdgTree.AddEdge(0, 3)
        mdgTree.AddEdge(1, 4)
        mdgTree.AddEdge(1, 5)
        mdgTree.AddEdge(2, 6)
        mdgTree.AddEdge(2, 7)
        mdgTree.AddEdge(3, 8)
        mdgTree.AddEdge(3, 9)

        # Undirected graph with parallel edges
        # and self-loops.
        mug.AddEdge(0, 0)
        mug.AddEdge(0, 1)
        mug.AddEdge(1, 0)
        mug.AddEdge(1, 2)
        mug.AddEdge(1, 3)
        mug.AddEdge(4, 5)
        mug.AddEdge(4, 5)
        mug.AddEdge(6, 7)
        mug.AddEdge(7, 7)

        w = il.vtkGraphWriter()

        tmpdir = vtkGetTempDir()
        fname = tmpdir+"/testgrapwriread.vtk"

        w.SetFileName(fname)
        w.SetInputData(mdgTree)
        w.Write()

        r = il.vtkGraphReader()
        r.SetFileName(fname)
        r.Update()

        g = r.GetOutput()

        self.assertTrue(g.IsA("vtkDirectedGraph"))
        self.assertTrue(g.GetNumberOfVertices() == 10)
        self.assertTrue(g.GetNumberOfEdges() == 9)
        self.assertTrue(g.GetTargetVertex(0) == 1)

        del(r)
        import gc
        gc.collect()
        os.remove(fname)

        w.SetFileName(fname)
        w.SetInputData(mug)
        w.Write()

        r = il.vtkGraphReader()
        r.SetFileName(fname)
        r.Update()

        g = r.GetOutput()

        self.assertTrue(g.IsA("vtkUndirectedGraph"))
        self.assertTrue(g.GetNumberOfVertices() == 10)
        self.assertTrue(g.GetNumberOfEdges() == 9)
        self.assertTrue(g.GetTargetVertex(0) == 0)

        del(r)
        import gc
        gc.collect()
        os.remove(fname)

if __name__ == "__main__":
    Testing.main([(TestGraphWriterReader, 'test')])
