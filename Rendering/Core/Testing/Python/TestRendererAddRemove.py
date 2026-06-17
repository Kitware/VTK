#!/usr/bin/env python
# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

"""
TestRendererAddRemove:
Creates a render window with 9 viewports arranged in a 3x3 grid, each
containing a distinctly coloured cone on a burgundy background. The grid
positions are fixed: renderer i always occupies column (i % 3) and
row (i / 3), regardless of which other renderers are present.
3 randomly chosen renderers are removed from the render window. The
remaining 6 renderers keep their original grid positions and their cameras
are reset to fit the unchanged cell size.
Re-renders and verifies that each remaining viewport contains a visible
(non-background) cone.
An interactive keypress callback ('a'/'d') allows adding/removing renderers
one at a time when the test is run with the -I flag; grid positions are
preserved throughout.
"""

import os
import random
from pathlib import Path

import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.vtkCommonCore import vtkCommand, vtkUnsignedCharArray
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
)

VTK_DATA_ROOT = vtkGetDataRoot()

# Constants
GRID_COLS = 3
GRID_ROWS = 3
N = GRID_COLS * GRID_ROWS  # 9
N_REMOVE = 3

# Burgundy background: RGB (128, 0, 32) / 255 ≈ (0.502, 0.000, 0.125).
BG_R, BG_G, BG_B = 0.502, 0.000, 0.125

# Distinct cone colours – one per renderer.
COLORS = [
    (1.0, 0.2, 0.2),  # red
    (0.2, 1.0, 0.2),  # green
    (0.2, 0.2, 1.0),  # blue
    (1.0, 1.0, 0.2),  # yellow
    (0.2, 1.0, 1.0),  # cyan
    (1.0, 0.2, 1.0),  # magenta
    (1.0, 0.6, 0.2),  # orange
    (0.6, 0.2, 1.0),  # violet
    (0.2, 1.0, 0.6),  # mint
]


# Helpers
def grid_viewport(i):
    """Return the normalised viewport (xmin, ymin, xmax, ymax) for grid index i.

    Columns run left-to-right; rows run top-to-bottom (VTK y-axis is
    bottom-up, so row 0 maps to the top third of the window).
    """
    col = i % GRID_COLS
    row = i // GRID_COLS
    xmin = col / GRID_COLS
    xmax = (col + 1) / GRID_COLS
    # Invert rows so that index 0 is top-left.
    ymin = (GRID_ROWS - row - 1) / GRID_ROWS
    ymax = (GRID_ROWS - row) / GRID_ROWS
    return xmin, ymin, xmax, ymax


def create_cone_renderer(i, color):
    """Create a renderer containing a cone actor placed in grid cell i."""
    xmin, ymin, xmax, ymax = grid_viewport(i)

    cone = vtkConeSource()
    cone.SetHeight(3.0)
    cone.SetRadius(1.0)
    cone.SetResolution(20)

    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(cone.GetOutputPort())

    actor = vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(*color)

    renderer = vtkRenderer()
    renderer.AddActor(actor)
    renderer.SetBackground(BG_R, BG_G, BG_B)
    renderer.SetViewport(xmin, ymin, xmax, ymax)
    renderer.ResetCamera()
    return renderer


def viewport_has_cone(renWin, xmin, ymin, xmax, ymax, tolerance=15):
    """Sample the central 10 % of the viewport and return True if at least
    one pixel differs from the burgundy background by more than *tolerance*
    in any channel.
    """
    win_w, win_h = renWin.GetSize()

    cx = (xmin + xmax) * 0.5
    cy = (ymin + ymax) * 0.5
    half_dx = (xmax - xmin) * 0.05
    half_dy = (ymax - ymin) * 0.05

    x0 = int((cx - half_dx) * win_w)
    y0 = int((cy - half_dy) * win_h)
    x1 = int((cx + half_dx) * win_w)
    y1 = int((cy + half_dy) * win_h)

    pixels = vtkUnsignedCharArray()
    renWin.GetRGBACharPixelData(x0, y0, x1, y1, 0, pixels)

    bg = (int(BG_R * 255), int(BG_G * 255), int(BG_B * 255))
    for i in range(pixels.GetNumberOfTuples()):
        r = int(pixels.GetComponent(i, 0))
        g = int(pixels.GetComponent(i, 1))
        b = int(pixels.GetComponent(i, 2))
        if (
            abs(r - bg[0]) > tolerance
            or abs(g - bg[1]) > tolerance
            or abs(b - bg[2]) > tolerance
        ):
            return True
    return False


def redistribute_viewports(entries):
    """Restore every active renderer to its fixed grid cell and reset its
    camera so the cone fits the (unchanged) cell dimensions.
    """
    for entry in entries:
        if not entry["is_active"]:
            continue
        vp = grid_viewport(entry["original_index"])
        entry["renderer"].SetViewport(*vp)
        entry["renderer"].ResetCamera()


# Keypress callback
class KeypressCallback:
    """'d'/'D' – remove a randomly chosen active renderer.
    'a'/'A' – re-add the first removed renderer at its original grid cell.
    Grid positions are preserved throughout.
    """

    def __init__(self, renWin, entries, rng):
        self._ren_win = renWin
        self._entries = entries
        self._rng = rng

    def __call__(self, caller, event):
        key = caller.GetKeyCode()

        if key in ("d", "D"):
            active = [e for e in self._entries if e["is_active"]]
            if not active:
                print("No active renderers left to remove.")
                return
            entry = self._rng.choice(active)
            entry["is_active"] = False
            self._ren_win.RemoveRenderer(entry["renderer"])
            active_count = sum(1 for e in self._entries if e["is_active"])
            print(
                f"Removed renderer {entry['original_index']}."
                f" Active renderers: {active_count}"
            )

        elif key in ("a", "A"):
            inactive = [e for e in self._entries if not e["is_active"]]
            if not inactive:
                print("All renderers are already active.")
                return
            # Re-add the first removed renderer (lowest original index).
            entry = min(inactive, key=lambda e: e["original_index"])
            entry["is_active"] = True
            self._ren_win.AddRenderer(entry["renderer"])
            active_count = sum(1 for e in self._entries if e["is_active"])
            print(
                f"Added renderer {entry['original_index']}."
                f" Active renderers: {active_count}"
            )

        else:
            return

        redistribute_viewports(self._entries)
        self._ren_win.Render()


class TestRendererAddRemove(Testing.vtkTest):

    def test_renderer_add_remove(self):
        renWin = vtkRenderWindow()
        renWin.SetWindowName("TestRendererAddRemove")
        renWin.SetSize(300, 300)
        renWin.SetMultiSamples(0)

        iren = vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        rng = random.Random(42)

        entries = []
        for i in range(N):
            renderer = create_cone_renderer(i, COLORS[i])
            renWin.AddRenderer(renderer)
            entries.append(
                {
                    "renderer": renderer,
                    "original_index": i,
                    "is_active": True,
                }
            )

        renWin.Render()

        print(f"Verifying all {N} viewports after initial render...")
        for i in range(N):
            vp = grid_viewport(i)
            self.assertTrue(
                viewport_has_cone(renWin, *vp),
                f"Cone not visible in initial viewport {i}",
            )
        print(f"  All {N} viewports OK.")

        remove_entries = rng.sample(entries, N_REMOVE)
        remove_indices = sorted(e["original_index"] for e in remove_entries)
        print(
            f"Removing {N_REMOVE} randomly chosen renderers"
            f" (indices: {remove_indices})..."
        )

        for entry in remove_entries:
            renWin.RemoveRenderer(entry["renderer"])
            entry["is_active"] = False

        remaining = renWin.GetRenderers().GetNumberOfItems()
        expected = N - N_REMOVE
        self.assertEqual(
            remaining,
            expected,
            f"Expected {expected} renderers after removal, got {remaining}",
        )
        print(f"  Renderer count after removal: {remaining}")

        print("Restoring remaining renderers to their grid positions...")
        redistribute_viewports(entries)
        renWin.SetSize(299, 301)  # Force a resize to trigger viewport update.
        renWin.Render()
        renWin.SetSize(300, 300)
        renWin.Render()

        print(
            f"Verifying {expected} remaining viewports after redistribution...")
        for entry in entries:
            if not entry["is_active"]:
                continue
            vp = grid_viewport(entry["original_index"])
            self.assertTrue(
                viewport_has_cone(renWin, *vp),
                f"Cone not visible in viewport for renderer"
                f" {entry['original_index']} {vp}",
            )
        print(f"  All {expected} remaining viewports OK.")

        # Register keypress callback for interactive mode (-I flag).
        #  'd' / 'D'  remove a randomly chosen active renderer
        #  'a' / 'A'  re-add the first removed renderer
        cb = KeypressCallback(renWin, entries, rng)
        iren.AddObserver(vtkCommand.KeyPressEvent, cb)

        iren.Initialize()
        # Testing with image comparison
        img_file = "TestRendererAddRemove.png"
        Testing.compareImage(
            iren.GetRenderWindow(), Testing.getAbsImagePath(img_file), threshold=25
        )
        Testing.interact()


if __name__ == "__main__":
    Testing.main([(TestRendererAddRemove, "test")])
