r"""
    This module is a VTK Web server application.
    The following command line illustrate how to use it::

        $ vtkpython .../vtk_web_graph.py --vertices 1000 --edges 400

    Any VTK Web executable script come with a set of standard arguments that
    can be overriden if need be::
        --host localhost
             Interface on which the HTTP server will listen on.

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey vtk-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "vtk-secret" as secret key.
"""

# import to process args
import sys
import os

# import vtk modules.
from vtk import *
import json
import math

# import vtk web modules
from vtk.web import server, wamp, protocols

# import annotations
from autobahn.wamp import exportRpc

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom File Opener class to handle clients requests
# =============================================================================

class _WebGraph(wamp.ServerProtocol):

    # Application configuration
    vertices = 1000
    edges    = 400
    view     = None
    authKey  = "vtkweb-secret"

    def initialize(self):
        global renderer, renderWindow, renderWindowInteractor, cone, mapper, actor

        # Bring used components
        self.registerVtkWebProtocol(protocols.vtkWebMouseHandler())
        self.registerVtkWebProtocol(protocols.vtkWebViewPort())
        self.registerVtkWebProtocol(protocols.vtkWebViewPortImageDelivery())

        # Update authentication key to use
        self.updateSecret(_WebGraph.authKey)

        # Create default pipeline (Only once for all the sessions)
        if not _WebGraph.view:
            # Generate Random graph
            random = vtkRandomGraphSource()
            random.SetNumberOfVertices(_WebGraph.vertices)
            random.SetNumberOfEdges(_WebGraph.edges)
            random.SetStartWithTree(True)
            random.Update()
            graphData = random.GetOutput()

            # Create view
            view = vtkGraphLayoutView()
            view.AddRepresentationFromInput(graphData)

            # Customize Rendering
            view.SetVertexLabelArrayName("vertex id")
            view.SetVertexLabelVisibility(True)
            view.SetVertexColorArrayName("vertex id")
            view.SetColorVertices(True)
            view.SetScalingArrayName("vertex id")
            view.ScaledGlyphsOn()
            view.HideVertexLabelsOnInteractionOn()
            view.SetEdgeColorArrayName("edge id")
            view.SetColorEdges(True)

            view.SetLayoutStrategyToSpanTree()

            # Set trackball interaction style
            style = vtkInteractorStyleTrackballCamera()
            view.GetRenderWindow().GetInteractor().SetInteractorStyle(style)

            # VTK Web application specific
            _WebGraph.view = view
            view.ResetCamera()
            view.Render()
            self.Application.GetObjectIdMap().SetActiveObject("VIEW", view.GetRenderWindow())

    @exportRpc("changeLayout")
    def changeLayout(self, layoutName):
        if  layoutName == 'ForceDirected' :
            print 'Layout Strategy = Force Directed'
            _WebGraph.view.SetLayoutStrategyToForceDirected()
            _WebGraph.view.GetLayoutStrategy().ThreeDimensionalLayoutOn()
        if  layoutName == 'SpanTree' :
            print 'Layout Strategy = Span Tree (Depth First Off)'
            _WebGraph.view.SetLayoutStrategyToSpanTree()
            _WebGraph.view.GetLayoutStrategy().DepthFirstSpanningTreeOff()
        elif layoutName == 'SpanTreeDepthFirst' :
            print 'Layout Strategy = Span Tree (Depth First On)'
            _WebGraph.view.SetLayoutStrategyToSpanTree()
            _WebGraph.view.GetLayoutStrategy().DepthFirstSpanningTreeOn()
        elif layoutName == 'Circular' :
            print 'Layout Strategy = Circular'
            _WebGraph.view.SetLayoutStrategyToCircular()
        elif layoutName == 'Random' :
            print 'Layout Strategy = Random'
            _WebGraph.view.SetLayoutStrategyToRandom()
        elif layoutName == 'Fast2D' :
            print 'Layout Strategy = Fast 2D'
            _WebGraph.view.SetLayoutStrategyToFast2D()
        elif layoutName == 'Clustering2D' :
            print 'Layout Strategy = Clustering 2D'
            _WebGraph.view.SetLayoutStrategyToClustering2D()
        elif layoutName == 'Community2D' :
            print 'Layout Strategy = Community 2D'
            _WebGraph.view.SetLayoutStrategyToCommunity2D()
        _WebGraph.view.ResetCamera()
        _WebGraph.view.Render()

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="VTK/Web Graph web-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--vertices", help="Number of vertices used to generate graph", dest="vertices", type=int, default=1000)
    parser.add_argument("--edges", help="Number of edges used to generate graph", dest="edges", type=int, default=400)

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _WebGraph.authKey  = args.authKey
    _WebGraph.vertices = args.vertices
    _WebGraph.edges    = args.edges

    # Start server
    server.start_webserver(options=args, protocol=_WebGraph)
