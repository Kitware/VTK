#!/usr/bin/env python
"""Volume render an OME-Zarr image a brick at a time with the WebGPU volume mapper.

The mapper reads a volume through the pipeline a brick at a time when its source can produce
sub-extents, so only the parts of the image the camera sees are ever read, and the picture fills in
over the frames that follow. This script supplies such a source for OME-Zarr (0.4 or 0.5, local
or remote) and renders with it.

    python StreamOMEZarr.py path/to/image.ome.zarr
    python StreamOMEZarr.py --synthetic demo.ome.zarr      # write a demo image first

Requires the ``zarr`` package (3.x). Run with ``--offscreen out.png`` to render until the volume is
complete, save the picture and print what was read, without opening a window.
"""

import argparse
import collections
import sys
import time

import numpy as np
import zarr

from vtkmodules.util.numpy_support import get_vtk_array_type, numpy_to_vtk
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkDataSetAttributes,
    vtkImageData,
    vtkPiecewiseFunction,
)
from vtkmodules.vtkCommonExecutionModel import (
    vtkAlgorithm,
    vtkStreamingDemandDrivenPipeline,
)
from vtkmodules.vtkRenderingCore import (
    vtkColorTransferFunction,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkVolume,
    vtkVolumeProperty,
    vtkWindowToImageFilter,
)
from vtkmodules.vtkRenderingVolume import vtkGPUVolumeRayCastMapper
from vtkmodules.vtkIOImage import vtkPNGWriter
import vtkmodules.vtkInteractionStyle  # noqa: F401
import vtkmodules.vtkRenderingVolumeWebGPU  # noqa: F401
from vtkmodules.vtkRenderingVolumeWebGPU import (
    vtkWebGPUVolumeRayCastCoordinator,
    vtkWebGPUVolumeRayCastMapper,
)

SDDP = vtkStreamingDemandDrivenPipeline


class OMEZarrSource(VTKPythonAlgorithmBase):
    """One channel of one time point of one resolution level of an OME-Zarr image.

    Publishes what lets the volume mapper stream it: that it can produce any sub-extent, the whole
    extent and geometry, the array type, and a scalar range, which OME-Zarr carries in its omero
    rendering settings or which is otherwise taken from the coarsest level, cheaply.
    """

    def __init__(self, path, level=0, channel=0, timepoint=0, cached_chunks=512):
        VTKPythonAlgorithmBase.__init__(
            self, nInputPorts=0, nOutputPorts=1, outputType="vtkImageData"
        )
        self._root = zarr.open_group(path, mode="r")
        attrs = dict(self._root.attrs)
        ome = attrs.get("ome", attrs)  # 0.5 nests the metadata under "ome"
        multiscale = ome["multiscales"][0]
        dataset = multiscale["datasets"][level]
        self._array = self._root[dataset["path"]]
        self._axes = [a["name"] if isinstance(a, dict) else a for a in multiscale["axes"]]
        self._fixed = {"t": timepoint, "c": channel}

        scale = [1.0] * len(self._axes)
        translation = [0.0] * len(self._axes)
        transforms = list(multiscale.get("coordinateTransformations", [])) + list(
            dataset.get("coordinateTransformations", [])
        )
        for transform in transforms:
            if transform["type"] == "scale":
                scale = [s * f for s, f in zip(scale, transform["scale"])]
            elif transform["type"] == "translation":
                translation = [t + f for t, f in zip(translation, transform["translation"])]
        index = {name: i for i, name in enumerate(self._axes)}
        self._spacing = tuple(scale[index[a]] for a in "xyz")
        self._origin = tuple(translation[index[a]] for a in "xyz")
        self._size = tuple(self._array.shape[index[a]] for a in "xyz")
        self._name = "intensity"
        self._range = self._published_range(ome, multiscale, channel)

        # Bricks carry a one voxel apron, so neighbouring bricks read into each other's chunks.
        # Keeping decoded chunks means each is decoded once however many bricks touch it.
        self._chunks = collections.OrderedDict()
        self._cached_chunks = cached_chunks
        self.reads = 0

    # --- geometry --------------------------------------------------------------------------
    def _published_range(self, ome, multiscale, channel):
        channels = ome.get("omero", {}).get("channels", [])
        if channel < len(channels):
            window = channels[channel].get("window", {})
            low = window.get("min", window.get("start"))
            high = window.get("max", window.get("end"))
            if low is not None and high is not None and high > low:
                return float(low), float(high)
        coarsest = self._root[multiscale["datasets"][-1]["path"]]
        values = np.asarray(coarsest[self._selection(coarsest, (slice(None),) * 3)])
        return float(values.min()), float(max(values.max(), values.min() + 1))

    def _selection(self, array, xyz):
        """Index `array`, whose axes are in file order, with slices given x, y, z."""
        picks = {"x": xyz[0], "y": xyz[1], "z": xyz[2]}
        return tuple(picks.get(a, self._fixed.get(a, 0)) for a in self._axes)

    # --- reading ---------------------------------------------------------------------------
    def _read(self, x0, x1, y0, y1, z0, z1):
        """Voxels [x0..x1] x [y0..y1] x [z0..z1] as a z, y, x array, assembled from chunks."""
        index = {name: i for i, name in enumerate(self._axes)}
        chunk = tuple(self._array.chunks[index[a]] for a in "xyz")
        out = np.empty((z1 - z0 + 1, y1 - y0 + 1, x1 - x0 + 1), dtype=self._array.dtype)
        for cz in range(z0 // chunk[2], z1 // chunk[2] + 1):
            for cy in range(y0 // chunk[1], y1 // chunk[1] + 1):
                for cx in range(x0 // chunk[0], x1 // chunk[0] + 1):
                    block = self._chunk(cx, cy, cz, chunk)
                    bx0, by0, bz0 = cx * chunk[0], cy * chunk[1], cz * chunk[2]
                    sx = slice(max(x0, bx0), min(x1, bx0 + block.shape[2] - 1) + 1)
                    sy = slice(max(y0, by0), min(y1, by0 + block.shape[1] - 1) + 1)
                    sz = slice(max(z0, bz0), min(z1, bz0 + block.shape[0] - 1) + 1)
                    out[sz.start - z0 : sz.stop - z0, sy.start - y0 : sy.stop - y0,
                        sx.start - x0 : sx.stop - x0] = block[
                        sz.start - bz0 : sz.stop - bz0, sy.start - by0 : sy.stop - by0,
                        sx.start - bx0 : sx.stop - bx0]
        return out

    def _chunk(self, cx, cy, cz, chunk):
        key = (cx, cy, cz)
        if key in self._chunks:
            self._chunks.move_to_end(key)
            return self._chunks[key]
        xyz = tuple(
            slice(c * n, min((c + 1) * n, size)) for c, n, size in zip(key, chunk, self._size)
        )
        block = np.asarray(self._array[self._selection(self._array, xyz)])
        # The array's own axis order may not be z, y, x.
        order = [a for a in self._axes if a in "xyz"]
        block = np.transpose(block, [order.index(a) for a in "zyx"])
        self._chunks[key] = block
        self.reads += 1
        if len(self._chunks) > self._cached_chunks:
            self._chunks.popitem(last=False)
        return block

    # --- pipeline --------------------------------------------------------------------------
    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        info.Set(SDDP.WHOLE_EXTENT(), (0, self._size[0] - 1, 0, self._size[1] - 1, 0,
                                       self._size[2] - 1), 6)
        info.Set(vtkDataObject.SPACING(), self._spacing, 3)
        info.Set(vtkDataObject.ORIGIN(), self._origin, 3)
        info.Set(vtkAlgorithm.CAN_PRODUCE_SUB_EXTENT(), 1)
        vtkDataObject.SetPointDataActiveScalarInfo(
            info, get_vtk_array_type(self._array.dtype), 1)
        field = vtkDataObject.GetActiveFieldInformation(
            info, vtkDataObject.FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes.SCALARS)
        field.Set(vtkDataObject.FIELD_NAME(), self._name)
        field.Set(vtkDataObject.FIELD_RANGE(), self._range, 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        extent = info.Get(SDDP.UPDATE_EXTENT())
        output = vtkImageData.GetData(outInfo)
        output.SetExtent(extent)
        output.SetSpacing(self._spacing)
        output.SetOrigin(self._origin)
        if extent[0] > extent[1] or extent[2] > extent[3] or extent[4] > extent[5]:
            return 1  # asked for nothing, which is what the mapper asks between bricks
        voxels = self._read(*extent)
        array = numpy_to_vtk(voxels.ravel(), deep=True)
        array.SetName(self._name)
        output.GetPointData().SetScalars(array)
        return 1

    @property
    def scalar_range(self):
        return self._range


def write_synthetic(path, size=384, chunk=64, levels=3):
    """A small multiscale OME-Zarr 0.4 image: nested shells with a little noise."""
    root = zarr.open_group(path, mode="w", zarr_format=2)
    z, y, x = np.mgrid[0:size, 0:size, 0:size].astype(np.float32) / (size - 1) - 0.5
    radius = np.sqrt(x * x + y * y + z * z)
    shells = 0.5 + 0.5 * np.cos(radius * 38.0)
    volume = np.where(radius < 0.46, shells * (1.0 - radius), 0.0)
    rng = np.random.default_rng(0)
    volume = (volume * 3000 + rng.normal(0, 40, volume.shape)).clip(0, 4095).astype(np.uint16)
    datasets = []
    for level in range(levels):
        data = volume[:: 2**level, :: 2**level, :: 2**level]
        root.create_array(str(level), data=data, chunks=(chunk, chunk, chunk))
        datasets.append({"path": str(level), "coordinateTransformations": [
            {"type": "scale", "scale": [float(2**level)] * 3}]})
    root.attrs["multiscales"] = [{
        "version": "0.4", "name": "synthetic",
        "axes": [{"name": a, "type": "space", "unit": "micrometer"} for a in "zyx"],
        "datasets": datasets}]
    root.attrs["omero"] = {"channels": [{"label": "shells", "window": {
        "min": 0, "max": 4095, "start": 0, "end": 4095}}]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("path", help="an OME-Zarr image")
    parser.add_argument("--synthetic", action="store_true", help="write a demo image to PATH")
    parser.add_argument("--level", type=int, default=0, help="resolution level, 0 is finest")
    parser.add_argument("--channel", type=int, default=0)
    parser.add_argument("--bricks-per-frame", type=int, default=64,
                        help="bricks read per frame; lower keeps each frame short")
    parser.add_argument("--offscreen", metavar="PNG",
                        help="render until complete, save the picture and exit")
    args = parser.parse_args()

    if args.synthetic:
        write_synthetic(args.path)

    source = OMEZarrSource(args.path, level=args.level, channel=args.channel)
    low, high = source.scalar_range

    mapper = vtkGPUVolumeRayCastMapper()
    mapper.SetInputConnection(source.GetOutputPort())
    if not isinstance(mapper, vtkWebGPUVolumeRayCastMapper):
        sys.exit("The WebGPU volume mapper is required; run with "
                 "VTK_FACTORY_PREFER=RenderingBackend=WebGPU.")
    mapper.SetStreamingThresholdMB(0)  # stream whatever the size, for the sake of the example

    def at(fraction):
        return low + fraction * (high - low)

    colors = vtkColorTransferFunction()
    colors.AddRGBPoint(at(0.0), 0.0, 0.0, 0.0)
    colors.AddRGBPoint(at(0.15), 0.1, 0.35, 0.95)
    colors.AddRGBPoint(at(0.35), 0.95, 0.55, 0.15)
    colors.AddRGBPoint(at(0.6), 1.0, 1.0, 0.85)
    opacity = vtkPiecewiseFunction()
    opacity.AddPoint(at(0.0), 0.0)
    opacity.AddPoint(at(0.1), 0.0)
    opacity.AddPoint(at(0.6), 0.06)
    prop = vtkVolumeProperty()
    prop.SetColor(colors)
    prop.SetScalarOpacity(opacity)
    prop.SetInterpolationTypeToLinear()
    prop.ShadeOn()

    volume = vtkVolume()
    volume.SetMapper(mapper)
    volume.SetProperty(prop)
    renderer = vtkRenderer()
    renderer.AddVolume(volume)
    renderer.SetBackground(0.08, 0.08, 0.1)
    window = vtkRenderWindow()
    window.SetSize(900, 900)
    window.AddRenderer(renderer)
    renderer.ResetCamera()
    renderer.GetActiveCamera().Azimuth(25)
    renderer.GetActiveCamera().Elevation(20)
    renderer.ResetCameraClippingRange()

    coordinator = vtkWebGPUVolumeRayCastCoordinator.GetForRenderer(renderer)
    coordinator.SetMaximumBricksPerFrame(args.bricks_per_frame)

    if args.offscreen:
        window.SetOffScreenRendering(True)
        start, frames = time.perf_counter(), 0
        while True:
            window.Render()
            frames += 1
            if coordinator.GetComplete() or frames > 1000:
                break
        seconds = time.perf_counter() - start
        grab = vtkWindowToImageFilter()
        grab.SetInput(window)
        writer = vtkPNGWriter()
        writer.SetFileName(args.offscreen)
        writer.SetInputConnection(grab.GetOutputPort())
        writer.Write()
        print("%s: %dx%dx%d voxels, complete after %d frames in %.2f s, %d chunks decoded"
              % (args.path, *source._size, frames, seconds, source.reads))
        return

    interactor = vtkRenderWindowInteractor()
    interactor.SetRenderWindow(window)
    interactor.Initialize()

    # Keep drawing while bricks are still arriving, so the volume fills in without input.
    def keep_filling(obj, event):
        if not coordinator.GetComplete():
            window.Render()

    interactor.AddObserver(vtkCommand.TimerEvent, keep_filling)
    interactor.CreateRepeatingTimer(15)
    window.Render()
    interactor.Start()


if __name__ == "__main__":
    main()
