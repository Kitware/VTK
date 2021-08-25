## no-static-kits

Static builds no longer allow building with kits. The goal of a kit build is to
reduce the number of runtime libraries needed, but this is not relevant in a
static build anyways. Additionally, there are issues that arise with static kit
builds that are lingering and rather than leaving them around for users to
accidentally run into, VTK now only considers `VTK_ENABLE_KITS` in shared
builds.
