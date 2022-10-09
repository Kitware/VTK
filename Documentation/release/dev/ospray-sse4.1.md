## OSPRay disabled on older x86\_64 processors

OSPRay's ISPC backend requires SSE4.1 to work (it calls `abort()` if not
supported), so avoiding the crash on such old hardware (SSE4.1 is from 2006 in
the Core 2 line) is of more interest than potentially working with OptiX (which
is now hidden due to this check).
