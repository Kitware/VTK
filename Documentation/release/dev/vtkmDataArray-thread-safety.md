## vtkmDataArray: Fix thread-safety issues

The `vtkmDataArray`'s `GetTuple`/`GetComponent`/`SetTuple`/`SetComponent` functions used to be unsafe when used from
many threads. That was because an internal array handle unknown helper was being swaped to a reader/writer helper by
many threads the first time they would ask to read or write data. A mutex was added to protect that swap which that does
not affect performance for any subsequent reads or writes other than the first one.
