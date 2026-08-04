## macOS wheel builds now use `uv` for relocatable Python

CI now obtains relocatable Python interpreters for macOS wheel builds via
the `uv` `python-build-standalone` project instead of `relocatable-python`.
