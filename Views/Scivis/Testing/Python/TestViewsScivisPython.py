# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""The Python layer over ViewsScivis.

The C++ classes are covered by the tests beside this one.  What is checked here
is what only exists in Python: the override classes registered for this module,
the properties they add, the delegation to the object that owns a property, the
keyword arguments the constructors take, and the container behaviour of the
things the module holds sets of.
"""

import unittest

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkPartitionedDataSet,
    vtkPartitionedDataSetCollection,
)
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.util.views_scivis import (
    GridAxesRepresentation,
    ScivisView,
    SurfaceRepresentation,
    TextOverlayRepresentation,
    VolumeRepresentation,
)

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401  registers the rendering classes
import vtkmodules.vtkRenderingVolumeOpenGL2  # noqa: F401


def colored_surface():
    """A surface colored by an array, with the pipeline kept alive by the caller."""
    sphere = vtkSphereSource()
    elevation = vtkElevationFilter()
    elevation.SetInputConnection(sphere.GetOutputPort())
    representation = SurfaceRepresentation()
    representation.SetInputConnection(elevation.GetOutputPort())
    return sphere, elevation, representation


def offscreen_view(**kwargs):
    view = ScivisView(**kwargs)
    view.render_window.SetOffScreenRendering(True)
    return view


class TestOverridesAreRegistered(unittest.TestCase):
    """Constructing the wrapped class gives the Python class, not the bare one."""

    def test_view_and_representations(self):
        from vtkmodules.vtkViewsScivis import (
            vtkGridAxesRepresentation,
            vtkScivisView,
            vtkSurfaceRepresentation,
            vtkTextOverlayRepresentation,
        )

        self.assertIsInstance(vtkScivisView(), ScivisView)
        self.assertIsInstance(vtkSurfaceRepresentation(), SurfaceRepresentation)
        self.assertIsInstance(vtkGridAxesRepresentation(), GridAxesRepresentation)
        self.assertIsInstance(vtkTextOverlayRepresentation(), TextOverlayRepresentation)

    def test_components_reached_through_the_view(self):
        view = offscreen_view()
        # These are made in C++, so they are overridden only if the override is
        # registered for the class rather than applied at construction.
        self.assertEqual(len(view.scalar_bars), 0)
        self.assertEqual(len(view.lookup_table_manager), 0)


class TestNamedColors(unittest.TestCase):
    def test_surface(self):
        representation = SurfaceRepresentation()
        representation.color = "tomato"
        self.assertAlmostEqual(representation.color[0], 1.0, places=2)
        representation.edge_color = (0.0, 1.0, 0.0)
        self.assertEqual(tuple(representation.edge_color), (0.0, 1.0, 0.0))

    def test_text_and_axes(self):
        text = TextOverlayRepresentation()
        text.color = "white"
        self.assertEqual(tuple(text.color), (1.0, 1.0, 1.0))

        axes = GridAxesRepresentation()
        axes.label_color = "tomato"
        self.assertAlmostEqual(axes.label_color[0], 1.0, places=2)
        axes.title_color = (0.0, 0.0, 1.0)
        self.assertEqual(tuple(axes.title_color), (0.0, 0.0, 1.0))

    def test_a_name_nothing_knows_is_refused(self):
        with self.assertRaises(ValueError):
            SurfaceRepresentation().color = "not_a_color"


class TestDelegation(unittest.TestCase):
    """A property the class does not carry is routed to the object that owns it."""

    def test_surface_reaches_its_property_and_filter(self):
        representation = SurfaceRepresentation()
        representation.ambient = 0.3  # the actor's property
        self.assertEqual(representation.GetProperty().GetAmbient(), 0.3)

    def test_view_reaches_the_light_kit_and_renderer(self):
        view = offscreen_view()
        view.key_light_intensity = 0.8
        self.assertEqual(view.GetLightKit().GetKeyLightIntensity(), 0.8)

    def test_delegation_is_snake_case_only(self):
        # CamelCase would hand back API the C++ classes deliberately do not
        # offer -- GetActor() has a GetMapper(), which the representation does
        # not, and must not gain by being asked through Python.
        representation = SurfaceRepresentation()
        with self.assertRaises(AttributeError):
            representation.GetMapper

    def test_an_unknown_property_is_refused(self):
        with self.assertRaises(AttributeError):
            SurfaceRepresentation().no_such_property_at_all


class TestConstructorKeywords(unittest.TestCase):
    def test_properties_of_the_class(self):
        text = TextOverlayRepresentation(text="Frame 12", position=(20, 40), font_size=24)
        self.assertEqual(text.text, "Frame 12")
        self.assertEqual(text.font_size, 24)
        self.assertEqual(tuple(text.position), (20.0, 40.0))

    def test_properties_that_exist_only_in_python(self):
        # `size` is the override's, not the wrapped class's, which spells it
        # `window_size`.  A property with no counterpart in C++ must not be
        # handed to VTK's own constructor handling, which would reject it.
        from vtkmodules.vtkViewsScivis import vtkScivisView

        self.assertFalse(hasattr(vtkScivisView, "size"), "size is no longer Python-only")
        view = offscreen_view(size=(640, 480))
        self.assertEqual(tuple(view.size), (640, 480))

        # And one that does exist in C++ still goes the other way.
        text = TextOverlayRepresentation(color="white")
        self.assertEqual(tuple(text.color), (1.0, 1.0, 1.0))

    def test_delegated_properties(self):
        representation = SurfaceRepresentation(specular=0.4, opacity=0.8)
        self.assertEqual(representation.specular, 0.4)
        self.assertEqual(representation.opacity, 0.8)

    def test_a_dict_configures_a_sub_object(self):
        view = offscreen_view(
            selector={"mode": "frustum"},
            scalar_bars={"label_font_size": 22, "draggable": True},
        )
        self.assertEqual(view.scalar_bars.label_font_size, 22)
        self.assertTrue(view.scalar_bars.draggable)

    def test_a_misspelling_is_refused(self):
        for bad in (
            lambda: TextOverlayRepresentation(font_siz=20),
            lambda: SurfaceRepresentation(colr="tomato"),
            lambda: ScivisView(scalar_bars={"label_font_siz": 22}),
        ):
            with self.assertRaises(ValueError):
                bad()


class TestTheViewIsASequence(unittest.TestCase):
    def test_add_remove_and_read_back(self):
        view = offscreen_view()
        _sphere, _elevation, representation = colored_surface()

        self.assertEqual(len(view), 0)
        view += representation
        self.assertEqual(len(view), 1)
        self.assertIs(view[0], representation)
        self.assertEqual([type(r).__name__ for r in view], ["SurfaceRepresentation"])

        view -= representation
        self.assertEqual(len(view), 0)

    def test_past_the_end(self):
        with self.assertRaises(IndexError):
            offscreen_view()[0]


class TestScalarBarsAreAContainer(unittest.TestCase):
    def setUp(self):
        self.view = offscreen_view()
        self.sphere, self.elevation, self.representation = colored_surface()
        self.view += self.representation
        self.view.Render()

    def test_length_and_iteration(self):
        self.assertEqual(len(self.view.scalar_bars), 1)
        self.assertEqual([bar.title for bar in self.view.scalar_bars], ["Elevation"])

    def test_by_position_and_by_name(self):
        bars = self.view.scalar_bars
        self.assertIs(bars[0], bars.GetActor(0))
        self.assertIs(bars["Elevation"], bars[0])
        self.assertIs(
            bars["Elevation", vtkDataObject.FIELD_ASSOCIATION_POINTS], bars[0]
        )

    def test_membership(self):
        self.assertIn("Elevation", self.view.scalar_bars)
        self.assertNotIn("Nothing", self.view.scalar_bars)

    def test_missing_keys(self):
        with self.assertRaises(KeyError):
            self.view.scalar_bars["Nothing"]
        with self.assertRaises(IndexError):
            self.view.scalar_bars[7]


class TestLookupTableManagerIsAMapping(unittest.TestCase):
    def setUp(self):
        self.view = offscreen_view()
        self.manager = self.view.lookup_table_manager

    def test_set_get_and_membership(self):
        table = vtkLookupTable()
        table.SetRange(2, 5)
        self.manager["Temperature"] = table
        self.assertIn("Temperature", self.manager)
        self.assertIs(self.manager["Temperature"], table)
        self.assertEqual(len(self.manager), 1)
        self.assertEqual(list(self.manager), ["Temperature"])

    def test_reading_a_missing_name_makes_one(self):
        # The manager exists to hand out a map for any array, the way
        # collections.defaultdict does.
        self.assertNotIn("Pressure", self.manager)
        made = self.manager["Pressure"]
        self.assertIsNotNone(made)
        self.assertIn("Pressure", self.manager)

    def test_deleting(self):
        self.manager["Temperature"] = vtkLookupTable()
        del self.manager["Temperature"]
        self.assertNotIn("Temperature", self.manager)
        with self.assertRaises(KeyError):
            del self.manager["Temperature"]


class TestBlocksAreIndexedByBlock(unittest.TestCase):
    def setUp(self):
        partitions = vtkPartitionedDataSet()
        for i in range(3):
            sphere = vtkSphereSource()
            sphere.SetCenter(i * 3.0, 0.0, 0.0)
            sphere.Update()
            partitions.SetPartition(i, sphere.GetOutput())
        collection = vtkPartitionedDataSetCollection()
        collection.SetPartitionedDataSet(0, partitions)

        self.representation = SurfaceRepresentation()
        self.representation.SetInputDataObject(collection)
        self.representation.color = (0.2, 0.4, 0.6)
        self.representation.opacity = 0.75
        self.view = offscreen_view()
        self.view += self.representation
        self.view.Render()

    def test_setting_one_block(self):
        blocks = self.representation.GetBlocks()
        blocks[3].visibility = False
        blocks[3].color = "tomato"
        blocks[3].opacity = 0.5

        self.assertFalse(blocks[3].visibility)
        self.assertAlmostEqual(blocks[3].color[0], 1.0, places=2)
        self.assertEqual(blocks[3].opacity, 0.5)

    def test_an_untouched_block_reports_what_it_is_drawn_with(self):
        blocks = self.representation.GetBlocks()
        self.assertTrue(blocks[2].visibility)
        self.assertEqual(tuple(round(c, 2) for c in blocks[2].color), (0.2, 0.4, 0.6))
        self.assertEqual(blocks[2].opacity, 0.75)

    def test_a_negative_index(self):
        with self.assertRaises(IndexError):
            self.representation.GetBlocks()[-1]

    def test_not_iterable(self):
        # Every index makes a block, so iterating by index would never end.
        blocks = self.representation.GetBlocks()
        with self.assertRaises(TypeError):
            iter(blocks)
        with self.assertRaises(TypeError):
            list(blocks)
        with self.assertRaises(TypeError):
            blocks[0] in blocks


class TestShow(unittest.TestCase):
    def test_builds_configures_and_adds(self):
        view = offscreen_view()
        sphere = vtkSphereSource()
        representation = view.show(sphere, color="tomato", opacity=0.5, specular=0.3)

        self.assertIsInstance(representation, SurfaceRepresentation)
        self.assertIs(view[0], representation)
        self.assertAlmostEqual(representation.color[0], 1.0, places=2)
        self.assertEqual(representation.opacity, 0.5)
        self.assertEqual(representation.specular, 0.3)

    def test_by_name(self):
        view = offscreen_view()
        from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

        source = vtkRTAnalyticSource()
        representation = view.show(source, "volume", scalar_opacity_unit_distance=1.5)
        self.assertIsInstance(representation, VolumeRepresentation)
        self.assertEqual(representation.scalar_opacity_unit_distance, 1.5)


if __name__ == "__main__":
    unittest.main()
