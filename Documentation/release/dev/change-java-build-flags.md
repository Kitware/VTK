# Change Java build flags

We superseded `VTK_JAVA_SOURCE_VERSION` and `VTK_JAVA_TARGET_VERSION` cmake
flags by `VTK_JAVA_RELEASE_VERSION` which has the effect of the two former
flags when having the same value.

This was added in newer JDK javac versions which now provide the `-release`
flags which simplifies its usage by avoiding having to add other flags such
as `--boot-class-path/-source/-release`.
