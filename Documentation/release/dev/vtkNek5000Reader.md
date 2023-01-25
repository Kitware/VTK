## New parallel Nek5000 reader

Added a reader for the NEK5000 data format.  This reader was authored by Jean
Favre, at the Swiss National Supercomputing Center, and until now existed as
a ParaView plugin available [here](https://github.com/jfavre/ParaViewNek5000Plugin).

Moving this reader into VTK proper provides python bindings automatically, and
will simplify the deployment of the reader with ParaView.
