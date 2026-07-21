## Marshal contexts can allocate identifiers in descending order

`vtkMarshalContext` now provides `SetAllocateIdsDescending`. When enabled,
`MakeId` hands out identifiers counting down from the maximum value of
`vtkTypeUInt32` instead of counting up from 0.

The WebAssembly `vtkRemoteSession` enables this mode on its context. A remote
session mirrors a server-side context that allocates identifiers in ascending
order, so identifiers created for client-only objects (a render window's
interactor, hardware window, etc.) no longer collide with identifiers the
server assigns later. Previously, such collisions caused incoming states to be
applied to unrelated client objects after a client-side serialization, which
corrupted the scene.
