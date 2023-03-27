# Improve VTK SMP STDThread backend

A common, global, thread pool is shared between all SMP calls, so they no longer create threads.
Nested SMP calls no longer spawn additional threads.

These changes lead to much better performances for small SMP calls and a better scalability for nested calls!
