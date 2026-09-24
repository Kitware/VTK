"""Zero-copy exchange between VTK arrays and DLPack-speaking libraries.

DLPack is the interchange protocol used by CuPy, PyTorch, JAX, NumPy and
TensorFlow. Speaking it means a VTK array can be handed to any of them --
and taken back -- without copying, on the host or on a GPU.

    from vtkmodules.util.dlpack_support import vtk_to_dlpack, dlpack_to_vtk

    import cupy
    gpu = cupy.from_dlpack(vtk_to_dlpack(vtk_array))   # VTK -> CuPy
    back = dlpack_to_vtk(gpu)                          # CuPy -> VTK

Both directions are views. Nothing is copied, so both sides are looking at
the same memory, and a write through one is visible through the other.

Lifetime
--------
That only works if the memory outlives whoever is looking at it, which is
the part these protocols exist to get right.

Exporting builds the capsule around a ``vtkMemoryDescriptor``, which holds
a reference to the source array; the reference is dropped when the
consumer releases the tensor. So a VTK array stays alive for exactly as
long as a CuPy view of it does, even if nothing else refers to it.

Importing goes the other way: the imported array holds the DLPack capsule
open, so the CuPy array behind it cannot be collected while VTK is using
it.

Neither direction requires the caller to think about this, which is the
point -- an interop layer that needs a lifetime rule written in the docs
is one that will be got wrong.

Device support
--------------
Host arrays work with plain VTK. Device arrays require VTK built with
``VTK_ENABLE_VISKORES`` so that ``vtkmDataArrayFactory`` is available to
wrap device memory; importing a device tensor without it raises
``RuntimeError`` rather than silently copying to the host.

Metal tensors are imported as host arrays. A shared-storage MTLBuffer on
Apple Silicon is unified memory, so its pointer is host-readable and the
wrap is still zero-copy; Viskores has no Metal device adapter, so there is
nothing else it could be. Private-storage buffers have no host-readable
pointer, report null, and are rejected. VTK never *exports* Metal memory,
because no VTK array can hold it.

Context, device and streams
---------------------------
Three things this module does not carry, all of them worth knowing before
relying on it for device memory.

**Context.** DLPack identifies a device with two integers, a type and an
ordinal; there is nowhere in the protocol to put a context handle. CUDA and
HIP do not need one -- the runtime keeps an implicitly shared primary
context per device, so a pointer identifies itself and every consumer lands
in the same place. Level Zero does not work that way: a USM pointer is only
meaningful inside the ``ze_context_handle_t`` it was allocated from, and
recovering that requires a context you already hold. So ``kDLOneAPI`` maps
to the ``"level_zero"`` space and ``vtkmDataArrayFactory`` refuses it,
naming the space, rather than treating the pointer as something it is not.
Supporting Level Zero means finding somewhere for the context to travel
first; it is not a matter of adding a device adapter.

**Device ordinal.** ``DLPackArray(array, device_id=...)`` is taken from the
caller and reported verbatim by ``__dlpack_device__``. It is not derived
from where the memory actually lives, so on a multi-GPU host the default of
0 will mislabel memory that is not on device 0. Pass the right ordinal.

**Streams.** ``__dlpack__`` accepts DLPack's ``stream`` argument and ignores
it. That argument is how a consumer says "make this safe to read on my
stream", so a producer with asynchronous work still queued -- Viskores on a
Kokkos backend, for instance -- can hand over memory whose contents are not
there yet, and the race has no diagnostic. Synchronize before exporting if
anything asynchronous produced the data.
"""

import ctypes

from vtkmodules.vtkCommonCore import (
    VTK_CHAR,
    VTK_DOUBLE,
    VTK_FLOAT,
    VTK_INT,
    VTK_LONG,
    VTK_LONG_LONG,
    VTK_SHORT,
    VTK_SIGNED_CHAR,
    VTK_UNSIGNED_CHAR,
    VTK_UNSIGNED_INT,
    VTK_UNSIGNED_LONG,
    VTK_UNSIGNED_LONG_LONG,
    VTK_UNSIGNED_SHORT,
    vtkMemoryDescriptor,
)

__all__ = [
    "vtk_to_dlpack",
    "dlpack_to_vtk",
    "DLPackArray",
    "supported_device_types",
]

# ---------------------------------------------------------------------
# DLPack ABI
# ---------------------------------------------------------------------

kDLCPU = 1
kDLCUDA = 2
kDLCUDAHost = 3
kDLOpenCL = 4
kDLVulkan = 7
kDLMetal = 8
kDLROCM = 10
kDLROCMHost = 11
kDLCUDAManaged = 13
kDLOneAPI = 14

kDLInt = 0
kDLUInt = 1
kDLFloat = 2

# DLPack device type -> the memory-space name vtkMemoryDescriptor uses.
#
# Metal maps to "host", not to a space of its own. Viskores has no Metal
# device adapter and vtkDataArray::MemorySpace has no Metal value, so nothing
# in VTK could ever branch on one -- carrying it would be a label that implies
# a capability that does not exist. A shared-storage MTLBuffer on Apple
# Silicon is unified memory, so treating the pointer as host memory is not a
# compromise, it is what it is. (Private-storage buffers have no host-readable
# pointer at all; they report null and are rejected below.)
_DEVICE_TO_SPACE = {
    kDLCPU: "host",
    kDLCUDAHost: "host",
    kDLROCMHost: "host",
    kDLMetal: "host",
    kDLCUDA: "cuda",
    kDLCUDAManaged: "cuda",
    kDLROCM: "hip",
    kDLOneAPI: "level_zero",
}

# The spaces VTK can report on export. vtkDataArray::MemorySpace has exactly
# three values, and vtkmDataArray derives its space from the device the data
# sits on, so these are the only ones that can appear.
_SPACE_TO_DEVICE = {
    "host": kDLCPU,
    "cuda": kDLCUDA,
    "hip": kDLROCM,
}


# VTK type -> (DLPack type code, bits)
# `long` is 64-bit on LP64 (Linux, macOS) and 32-bit on LLP64 (Windows), so
# its width is asked for rather than assumed. It has to be here at all
# because numpy_to_vtk maps numpy's int64 to whichever VTK type is natively
# that size -- vtkLongArray here, vtkLongLongArray elsewhere -- so leaving it
# out breaks the round trip on exactly one class of platform.
_LONG_BITS = ctypes.sizeof(ctypes.c_long) * 8

_VTK_TO_DLPACK = {
    VTK_FLOAT: (kDLFloat, 32),
    VTK_DOUBLE: (kDLFloat, 64),
    VTK_SIGNED_CHAR: (kDLInt, 8),
    VTK_CHAR: (kDLInt, 8),
    VTK_SHORT: (kDLInt, 16),
    VTK_INT: (kDLInt, 32),
    VTK_LONG: (kDLInt, _LONG_BITS),
    VTK_LONG_LONG: (kDLInt, 64),
    VTK_UNSIGNED_CHAR: (kDLUInt, 8),
    VTK_UNSIGNED_SHORT: (kDLUInt, 16),
    VTK_UNSIGNED_INT: (kDLUInt, 32),
    VTK_UNSIGNED_LONG: (kDLUInt, _LONG_BITS),
    VTK_UNSIGNED_LONG_LONG: (kDLUInt, 64),
}

_DLPACK_TO_VTK = {
    (kDLFloat, 32): VTK_FLOAT,
    (kDLFloat, 64): VTK_DOUBLE,
    (kDLInt, 8): VTK_SIGNED_CHAR,
    (kDLInt, 16): VTK_SHORT,
    (kDLInt, 32): VTK_INT,
    (kDLInt, 64): VTK_LONG_LONG,
    (kDLUInt, 8): VTK_UNSIGNED_CHAR,
    (kDLUInt, 16): VTK_UNSIGNED_SHORT,
    (kDLUInt, 32): VTK_UNSIGNED_INT,
    (kDLUInt, 64): VTK_UNSIGNED_LONG_LONG,
}


class _DLDataType(ctypes.Structure):
    _fields_ = [("code", ctypes.c_uint8), ("bits", ctypes.c_uint8), ("lanes", ctypes.c_uint16)]


class _DLDevice(ctypes.Structure):
    _fields_ = [("device_type", ctypes.c_int32), ("device_id", ctypes.c_int32)]


class _DLTensor(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.c_void_p),
        ("device", _DLDevice),
        ("ndim", ctypes.c_int32),
        ("dtype", _DLDataType),
        ("shape", ctypes.POINTER(ctypes.c_int64)),
        ("strides", ctypes.POINTER(ctypes.c_int64)),
        ("byte_offset", ctypes.c_uint64),
    ]


class _DLManagedTensor(ctypes.Structure):
    _fields_ = [
        ("dl_tensor", _DLTensor),
        ("manager_ctx", ctypes.c_void_p),
        ("deleter", ctypes.CFUNCTYPE(None, ctypes.c_void_p)),
    ]


# DLPack v1.0. This is the only form that can say a buffer is read-only,
# and both directions need it: NumPy will only export a read-only array
# through it, and refuses to export at all over the legacy protocol -- so
# without this a round trip through NumPy is impossible.
#
# Note dl_tensor moves to the end. Reading one layout as the other
# misreads every field, so the two are never used interchangeably.
class _DLPackVersion(ctypes.Structure):
    _fields_ = [("major", ctypes.c_uint32), ("minor", ctypes.c_uint32)]


class _DLManagedTensorVersioned(ctypes.Structure):
    _fields_ = [
        ("version", _DLPackVersion),
        ("manager_ctx", ctypes.c_void_p),
        ("deleter", ctypes.CFUNCTYPE(None, ctypes.c_void_p)),
        ("flags", ctypes.c_uint64),
        ("dl_tensor", _DLTensor),
    ]


DLPACK_FLAG_BITMASK_READ_ONLY = 1 << 0


_PyCapsule_New = ctypes.pythonapi.PyCapsule_New
_PyCapsule_New.restype = ctypes.py_object
_PyCapsule_New.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]

# Raw pointers rather than py_object: the capsule destructor runs during
# teardown, where touching the capsule's own refcount can resurrect it.
_PyCapsule_IsValid = ctypes.pythonapi.PyCapsule_IsValid
_PyCapsule_IsValid.restype = ctypes.c_int
_PyCapsule_IsValid.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

_PyCapsule_GetPointer = ctypes.pythonapi.PyCapsule_GetPointer
_PyCapsule_GetPointer.restype = ctypes.c_void_p
_PyCapsule_GetPointer.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


# ---------------------------------------------------------------------
# Export lifetime
# ---------------------------------------------------------------------
#
# Everything the exported capsule points at has to outlive the consumer:
# the descriptor holding the VTK array open, the managed tensor, and the
# shape and stride arrays. They are pinned here under a key.
#
# The key travels in manager_ctx, because that is the only thing DLPack
# gives the deleter. Keying this table by anything the deleter cannot
# recover means nothing is ever released and every export leaks -- device
# memory included.

_pinned = {}
_next_key = 1  # 0 round-trips through c_void_p as None


def _release(managed_ptr, struct=_DLManagedTensor):
    """Drop what was pinned for the export at *managed_ptr*.

    *struct* says which layout to read: manager_ctx sits at a different
    offset in the two.
    """
    if not managed_ptr:
        return
    managed = ctypes.cast(managed_ptr, ctypes.POINTER(struct)).contents
    _pinned.pop(managed.manager_ctx, None)


@ctypes.CFUNCTYPE(None, ctypes.c_void_p)
def _tensor_deleter(managed_ptr):
    """Called by the consumer when it is done with the tensor."""
    _release(managed_ptr, _DLManagedTensor)


@ctypes.CFUNCTYPE(None, ctypes.c_void_p)
def _tensor_deleter_versioned(managed_ptr):
    """As above for a v1.0 tensor: the deleter is handed only a pointer
    and cannot tell the layouts apart, so there is one per layout."""
    _release(managed_ptr, _DLManagedTensorVersioned)


@ctypes.CFUNCTYPE(None, ctypes.c_void_p)
def _capsule_destructor(capsule_ptr):
    """Release an export no consumer ever adopted.

    A consumer that takes the tensor renames the capsule to
    "used_dltensor" and takes over calling the deleter. A capsule
    collected still named "dltensor" was nobody's responsibility.
    """
    if not capsule_ptr:
        return
    if _PyCapsule_IsValid(capsule_ptr, b"dltensor"):
        _release(_PyCapsule_GetPointer(capsule_ptr, b"dltensor"), _DLManagedTensor)
    elif _PyCapsule_IsValid(capsule_ptr, b"dltensor_versioned"):
        _release(
            _PyCapsule_GetPointer(capsule_ptr, b"dltensor_versioned"),
            _DLManagedTensorVersioned,
        )


def supported_device_types():
    """DLPack device types this module can import, as a set of ints."""
    return set(_DEVICE_TO_SPACE)


# ---------------------------------------------------------------------
# VTK -> DLPack
# ---------------------------------------------------------------------


def vtk_to_dlpack(array, device_id=0, versioned=False):
    """Export a vtkDataArray as a DLPack capsule, without copying.

    The tensor is 2-D, ``(number of tuples, number of components)``, which
    is how the array is laid out. Single-component arrays are still 2-D
    with a trailing 1; reshape on the consumer side if you want 1-D.

    The array is kept alive for as long as the consumer holds the tensor,
    so it is safe to drop every other reference to it.

    With *versioned*, produces a v1.0 tensor in a "dltensor_versioned"
    capsule instead of the legacy "dltensor" one. Consumers that ask for
    v1.0 need this: over the legacy protocol NumPy cannot tell whether
    writing is safe, so it marks the view read-only and then will not
    re-export it.

    Raises TypeError for a data type with no DLPack equivalent, and
    ValueError for an array with no accessible memory -- implicit and
    computed arrays have none to describe.
    """
    global _next_key

    dl_type = _VTK_TO_DLPACK.get(array.GetDataType())
    if dl_type is None:
        raise TypeError(
            f"{array.GetClassName()} has data type {array.GetDataTypeAsString()}, "
            f"which has no DLPack equivalent"
        )
    code, bits = dl_type

    descriptors = array.NewMemoryDescriptors()
    if descriptors is None or descriptors.GetNumberOfItems() == 0:
        raise ValueError(
            f"{array.GetClassName()} exposes no memory to describe. Implicit and "
            f"computed arrays have none; use vtkArrayDispatch or a deep copy."
        )
    if descriptors.GetNumberOfItems() > 1:
        raise ValueError(
            "DLPack describes one contiguous buffer, but this array reports "
            f"{descriptors.GetNumberOfItems()} (an SoA layout). Convert it to AoS "
            "first, or export each component separately."
        )

    descriptor = descriptors.GetItemAsObject(0)
    space = descriptor.GetMemorySpace() or "host"
    device_type = _SPACE_TO_DEVICE.get(space)
    if device_type is None:
        raise ValueError(f"unknown memory space {space!r}")

    n_tuples = array.GetNumberOfTuples()
    n_components = array.GetNumberOfComponents()

    shape = (ctypes.c_int64 * 2)(n_tuples, n_components)
    strides = (ctypes.c_int64 * 2)(n_components, 1)  # in elements, not bytes

    if versioned:
        managed = _DLManagedTensorVersioned()
        managed.version = _DLPackVersion(major=1, minor=0)
        managed.flags = 0  # a vtkDataArray is always writable
        capsule_name = b"dltensor_versioned"
        deleter = _tensor_deleter_versioned
    else:
        managed = _DLManagedTensor()
        capsule_name = b"dltensor"
        deleter = _tensor_deleter

    managed.dl_tensor.data = ctypes.c_void_p(descriptor.GetPointer())
    managed.dl_tensor.device = _DLDevice(device_type=device_type, device_id=device_id)
    managed.dl_tensor.ndim = 2
    managed.dl_tensor.dtype = _DLDataType(code=code, bits=bits, lanes=1)
    managed.dl_tensor.shape = shape
    managed.dl_tensor.strides = strides
    managed.dl_tensor.byte_offset = 0

    key = _next_key
    _next_key += 1
    managed.manager_ctx = ctypes.c_void_p(key)
    managed.deleter = deleter

    # The descriptor is what holds the VTK array open; keeping the
    # descriptor keeps the array. `descriptors` is kept too so the
    # collection does not take its items down with it.
    _pinned[key] = (descriptors, descriptor, managed, shape, strides)

    # addressof, not byref: byref yields a temporary not tied to the capsule.
    return _PyCapsule_New(
        ctypes.addressof(managed),
        capsule_name,
        ctypes.cast(_capsule_destructor, ctypes.c_void_p),
    )


class DLPackArray:
    """Adapter exposing a vtkDataArray through the DLPack protocol.

    Libraries call ``__dlpack__`` rather than taking a capsule directly, so
    wrapping the array lets them consume it as they would any other array:

        cupy.from_dlpack(DLPackArray(vtk_array))
        torch.from_dlpack(DLPackArray(vtk_array))
    """

    def __init__(self, array, device_id=0):
        self.array = array
        self.device_id = device_id

    def __dlpack__(self, *, stream=None, max_version=None, dl_device=None, copy=None):
        if copy is True:
            raise BufferError("vtk_to_dlpack exports a view; it cannot copy")
        versioned = max_version is not None and max_version[0] >= 1
        return vtk_to_dlpack(self.array, self.device_id, versioned=versioned)

    def __dlpack_device__(self):
        descriptors = self.array.NewMemoryDescriptors()
        if descriptors is None or descriptors.GetNumberOfItems() == 0:
            return (kDLCPU, 0)
        space = descriptors.GetItemAsObject(0).GetMemorySpace() or "host"
        return (_SPACE_TO_DEVICE.get(space, kDLCPU), self.device_id)


# ---------------------------------------------------------------------
# DLPack -> VTK
# ---------------------------------------------------------------------


def dlpack_to_vtk(source, name=None):
    """Import a DLPack tensor as a vtkDataArray, without copying.

    *source* may be a capsule or any object implementing ``__dlpack__`` --
    a CuPy array, a PyTorch tensor, a NumPy array.

    A 1-D tensor becomes a single-component array. A 2-D tensor becomes
    ``(tuples, components)``. Higher rank is rejected, since a
    vtkDataArray has no way to represent it.

    The tensor is held open for as long as the returned array lives, so the
    source can be dropped immediately.
    """
    # Host and device memory take the same route through the capsule. Handing
    # a host tensor to numpy.from_dlpack instead would be less code, but it
    # inherits whatever that numpy does with mutability: before 2.1 it has no
    # versioned protocol to ask for, so it marks *every* import read-only and
    # a perfectly writable tensor comes back unusable. The flag is in the
    # capsule, so it is read from there.
    capsule = _request_capsule(source)

    capsule_ptr = ctypes.cast(id(capsule), ctypes.c_void_p)
    if _PyCapsule_IsValid(capsule_ptr, b"dltensor_versioned"):
        capsule_name = b"dltensor_versioned"
        used_name = b"used_dltensor_versioned"
        struct = _DLManagedTensorVersioned
    elif _PyCapsule_IsValid(capsule_ptr, b"dltensor"):
        capsule_name = b"dltensor"
        used_name = b"used_dltensor"
        struct = _DLManagedTensor
    else:
        raise ValueError(
            "expected an unconsumed DLPack capsule; this one has already been "
            "taken by another consumer"
        )

    managed_ptr = _PyCapsule_GetPointer(capsule_ptr, capsule_name)
    managed = ctypes.cast(managed_ptr, ctypes.POINTER(struct)).contents
    tensor = managed.dl_tensor

    if struct is _DLManagedTensorVersioned:
        if managed.version.major > 1:
            raise ValueError(
                f"DLPack v{managed.version.major}.{managed.version.minor} tensor is "
                f"newer than this module understands (v1.0)"
            )
        if managed.flags & DLPACK_FLAG_BITMASK_READ_ONLY:
            # A vtkDataArray has no read-only state, so wrapping this would
            # hand out a writable array over memory the producer says must
            # not be written -- and the producer may have it on a const page.
            raise ValueError(
                "the tensor is read-only, and a vtkDataArray cannot represent "
                "that. Copy the source first if you want a VTK array from it."
            )

    if not tensor.data:
        raise ValueError(
            "the tensor has a null data pointer. A Metal buffer with private "
            "storage reports this, and its memory cannot be read from the host; "
            "use shared storage."
        )

    if tensor.ndim not in (1, 2):
        raise ValueError(
            f"a vtkDataArray is 2-D at most (tuples x components); got ndim={tensor.ndim}"
        )

    vtk_type = _DLPACK_TO_VTK.get((tensor.dtype.code, tensor.dtype.bits))
    if vtk_type is None:
        raise TypeError(
            f"no VTK type for DLPack dtype (code={tensor.dtype.code}, "
            f"bits={tensor.dtype.bits}, lanes={tensor.dtype.lanes})"
        )
    if tensor.dtype.lanes != 1:
        raise TypeError(f"vectorized DLPack dtypes are not supported (lanes={tensor.dtype.lanes})")

    n_tuples = tensor.shape[0]
    n_components = tensor.shape[1] if tensor.ndim == 2 else 1

    if tensor.strides:
        expected = (n_components, 1) if tensor.ndim == 2 else (1,)
        actual = tuple(tensor.strides[i] for i in range(tensor.ndim))
        if actual != expected:
            raise ValueError(
                f"only C-contiguous tensors can be wrapped without copying; "
                f"strides {actual} != {expected}. Make the source contiguous first."
            )

    space = _DEVICE_TO_SPACE.get(tensor.device.device_type)
    if space is None:
        raise ValueError(f"unsupported DLPack device type {tensor.device.device_type}")

    itemsize = tensor.dtype.bits // 8
    size_in_bytes = n_tuples * n_components * itemsize
    pointer = (tensor.data or 0) + tensor.byte_offset

    # From here the imported array owns the tensor: the source may be
    # dropped, and the deleter runs when VTK is finished. Registering renames
    # the capsule to mark it consumed, so no one else -- including its own
    # destructor -- releases it too.
    key = _register_adopted(capsule, managed_ptr, struct, used_name)

    if space == "host":
        array = _wrap_host(key, vtk_type, n_tuples, n_components, pointer, tensor.ndim)
    else:
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(pointer, size_in_bytes, space, "data")
        _hold_via_descriptor(descriptor, key)
        array = _wrap_descriptor(descriptor, vtk_type, n_tuples, n_components, space)
    if name is not None:
        array.SetName(name)
    return array


def _request_capsule(source):
    """Get a capsule from *source*, preferring the versioned protocol.

    Asking for v1.0 first matters: NumPy refuses to export a read-only
    array over the legacy protocol, because that protocol cannot say so.
    A producer predating v1.0 raises TypeError, so fall back.
    """
    if not hasattr(source, "__dlpack__"):
        return source
    try:
        return source.__dlpack__(max_version=(1, 0))
    except TypeError:
        pass
    # A legacy-only producer. NumPy before 2.1 is one, and it will not
    # export a read-only array this way at all -- the legacy tensor has
    # nowhere to record it, so rather than lie it raises BufferError. That
    # is the same refusal dlpack_to_vtk makes on the versioned read-only
    # flag, so it is reported the same way rather than as a BufferError
    # escaping from a layer the caller never asked about.
    try:
        return source.__dlpack__()
    except BufferError as exc:
        raise ValueError(
            f"the source refused to export over the legacy DLPack protocol "
            f"({exc}). A read-only tensor is the usual reason; a vtkDataArray "
            f"cannot represent one, so copy the source first."
        ) from exc


# Capsules adopted from consumers, keyed the same way as exports.
_adopted = {}
_next_adopted_key = 1


def _release_adopted(key):
    """Drop the hold on an adopted tensor, calling the producer's deleter."""
    entry = _adopted.pop(key, None)
    if entry is None:
        return
    _capsule, managed_ptr, struct = entry
    managed = ctypes.cast(managed_ptr, ctypes.POINTER(struct)).contents
    if managed.deleter:
        managed.deleter(managed_ptr)


@ctypes.CFUNCTYPE(None, ctypes.c_void_p)
def _adopted_release(context):
    """Called by VTK when it is finished with an imported tensor."""
    _release_adopted(context)


_PyCapsule_SetName = ctypes.pythonapi.PyCapsule_SetName
_PyCapsule_SetName.restype = ctypes.c_int
_PyCapsule_SetName.argtypes = [ctypes.py_object, ctypes.c_char_p]


def _register_adopted(capsule, managed_ptr, struct, used_name):
    """Take ownership of *capsule*, and return the key that releases it.

    Renaming the capsule is what marks it consumed: from here its own
    destructor leaves it alone, and releasing it is this module's job.
    """
    global _next_adopted_key

    _PyCapsule_SetName(capsule, used_name)

    key = _next_adopted_key
    _next_adopted_key += 1
    _adopted[key] = (capsule, managed_ptr, struct)
    return key


def _hold_via_descriptor(descriptor, key):
    """Release the tensor keyed *key* when *descriptor* is destroyed."""
    # SetReleaseAddress, not SetRelease: the latter takes a function
    # pointer, which the wrapping tools cannot express, so it does not
    # exist on the Python side at all.
    descriptor.SetReleaseAddress(
        ctypes.cast(_adopted_release, ctypes.c_void_p).value, key)


class _HostTensor:
    """Shows an imported host tensor to numpy, and holds it open.

    numpy.asarray() of this builds a view whose base is this object, so the
    tensor stays open for exactly as long as anything is looking at the
    memory, and is released once nothing is.

    The hold is this object's own lifetime, rather than the
    vtkMemoryDescriptor release callback the device path uses. That callback
    is a ctypes trampoline invoked from a C++ destructor, and a vtkObject can
    outlive Py_Finalize: reaching for the GIL then crashes inside
    PyGILState_Ensure instead of releasing anything. Python finalization runs
    while the interpreter is still up, and the worst it can do is leak.

    The array interface is what makes the view writable. Nothing here asks
    the producer about mutability; dlpack_to_vtk has already read the
    read-only flag off the capsule and refused if it was set.
    """

    def __init__(self, key, pointer, shape, dtype):
        self._key = key
        self.__array_interface__ = {
            "version": 3,
            "data": (pointer, False),  # False: writable
            "shape": shape,
            "typestr": dtype.str,
        }

    def __del__(self):
        _release_adopted(self._key)


def _wrap_host(key, vtk_type, n_tuples, n_components, pointer, ndim):
    """Build the array from a host pointer, via numpy.

    Host memory does not go through Viskores, and must not: plain VTK builds
    have no vtkmDataArrayFactory, and host tensors are the common case.
    numpy is only the vehicle -- numpy_to_vtk(deep=False) points a
    vtkAOSDataArrayTemplate at the buffer and keeps the view alive -- and the
    dtype comes from the same VTK type the device path derived, so the two
    agree on what the tensor holds.
    """
    import numpy

    from vtkmodules.util import numpy_support

    dtype = numpy.dtype(numpy_support.get_numpy_array_type(vtk_type))
    shape = (n_tuples, n_components) if ndim == 2 else (n_tuples,)
    view = numpy.asarray(_HostTensor(key, pointer, shape, dtype))
    return numpy_support.numpy_to_vtk(view, deep=False, array_type=vtk_type)


def _wrap_descriptor(descriptor, vtk_type, n_tuples, n_components, space):
    """Build the array from a device pointer, via Viskores."""

    try:
        from vtkmodules.vtkAcceleratorsVTKmCore import vtkmDataArrayFactory
    except ImportError as exc:
        raise RuntimeError(
            f"importing {space!r} memory needs VTK built with the Viskores "
            f"accelerators (-DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmCore=YES); "
            f"only host tensors can be wrapped otherwise"
        ) from exc

    factory = vtkmDataArrayFactory()
    factory.SetNumberOfTuples(n_tuples)
    factory.SetNumberOfComponents(n_components)
    factory.SetDataType(vtk_type)
    factory.AddBuffer(descriptor)
    array = factory.CreateArray()
    if array is None:
        raise RuntimeError(
            f"could not wrap {space!r} memory as a vtkmDataArray "
            f"({n_tuples} tuples x {n_components} components, VTK type {vtk_type})"
        )
    return array
