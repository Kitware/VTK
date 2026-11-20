## vtkSocket: Stop receiving after a failed recv() on POSIX

`vtkSocket::Receive` now reports an error and returns 0 when `recv()` fails
on Linux and macOS, as it already did on Windows. It used to add the -1 to
its byte count, so a later successful `recv()`, for example after a receive
timeout, wrote before the caller's buffer. A connection reset is logged as a
trace instead of an error.
