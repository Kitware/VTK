# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
"""
Run some benchmarks and tests
"""
import sys as _sys


def helloworld(comm, args=None, verbose=True):
    """
    Hello, World! using MPI
    """
    from . import MPI
    from optparse import OptionParser
    parser = OptionParser(prog="mpi4py helloworld")
    parser.add_option("-q", "--quiet", action="store_false",
                      dest="verbose", default=verbose)
    (options, args) = parser.parse_args(args)

    size = comm.Get_size()
    rank = comm.Get_rank()
    name = MPI.Get_processor_name()
    message = ("Hello, World! I am process %*d of %d on %s.\n"
               % (len(str(size - 1)), rank, size, name))
    comm.Barrier()
    if rank > 0:
        comm.Recv([None, 'B'], rank - 1)
    if options.verbose:
        _sys.stdout.write(message)
        _sys.stdout.flush()
    if rank < size - 1:
        comm.Send([None, 'B'], rank + 1)
    comm.Barrier()
    return message


def ringtest(comm, args=None, verbose=True):
    """
    Time a message going around the ring of processes
    """
    # pylint: disable=too-many-locals
    # pylint: disable=too-many-branches
    from . import MPI
    from array import array
    from optparse import OptionParser
    parser = OptionParser(prog="mpi4py ringtest")
    parser.add_option("-q", "--quiet", action="store_false",
                      dest="verbose", default=verbose)
    parser.add_option("-n", "--size", type="int", default=1, dest="size",
                      help="message size")
    parser.add_option("-s", "--skip", type="int", default=0, dest="skip",
                      help="number of warm-up iterations")
    parser.add_option("-l", "--loop", type="int", default=1, dest="loop",
                      help="number of iterations")
    (options, args) = parser.parse_args(args)

    def ring(comm, n=1, loop=1, skip=0):
        # pylint: disable=invalid-name
        # pylint: disable=bad-whitespace
        # pylint: disable=missing-docstring
        iterations = list(range((loop + skip)))
        size = comm.Get_size()
        rank = comm.Get_rank()
        source = (rank - 1) % size
        dest = (rank + 1) % size
        Sendrecv = comm.Sendrecv
        Send = comm.Send
        Recv = comm.Recv
        Wtime = MPI.Wtime
        sendmsg = array('B', [+42]) * n
        recvmsg = array('B', [0x0]) * n
        if size == 1:
            for i in iterations:
                if i == skip:
                    tic = Wtime()
                Sendrecv(sendmsg, dest, 0,
                         recvmsg, source, 0)
        else:
            if rank == 0:
                for i in iterations:
                    if i == skip:
                        tic = Wtime()
                    Send(sendmsg, dest, 0)
                    Recv(recvmsg, source, 0)
            else:
                sendmsg = recvmsg
                for i in iterations:
                    if i == skip:
                        tic = Wtime()
                    Recv(recvmsg, source, 0)
                    Send(sendmsg, dest, 0)
        toc = Wtime()
        if comm.rank == 0 and sendmsg != recvmsg:  # pragma: no cover
            import warnings
            import traceback
            try:
                warnings.warn("received message does not match!")
            except UserWarning:
                traceback.print_exc()
                comm.Abort(2)
        return toc - tic

    size = getattr(options, 'size', 1)
    loop = getattr(options, 'loop', 1)
    skip = getattr(options, 'skip', 0)
    comm.Barrier()
    elapsed = ring(comm, size, loop, skip)
    if options.verbose and comm.rank == 0:
        message = ("time for %d loops = %g seconds (%d processes, %d bytes)\n"
                   % (loop, elapsed, comm.size, size))
        _sys.stdout.write(message)
        _sys.stdout.flush()
    return elapsed


def main(args=None):
    "Entry-point for ``python -m mpi4py``"
    from optparse import OptionParser
    from . import __name__ as prog
    from . import __version__ as version
    parser = OptionParser(prog=prog, version='%prog ' + version,
                          usage="%prog [options] <command> [args]")
    parser.add_option("--no-threads",
                      action="store_false", dest="threads", default=True,
                      help="initialize MPI without thread support")
    parser.add_option("--thread-level", type="choice", metavar="LEVEL",
                      choices=["single", "funneled", "serialized", "multiple"],
                      action="store", dest="thread_level", default="multiple",
                      help="initialize MPI with required thread support")
    parser.add_option("--mpe",
                      action="store_true", dest="mpe", default=False,
                      help="use MPE for MPI profiling")
    parser.add_option("--vt",
                      action="store_true", dest="vt", default=False,
                      help="use VampirTrace for MPI profiling")
    parser.disable_interspersed_args()
    (options, args) = parser.parse_args(args)

    from . import rc, profile
    rc.threads = options.threads
    rc.thread_level = options.thread_level
    if options.mpe:
        profile('mpe', logfile='mpi4py')
    if options.vt:
        profile('vt', logfile='mpi4py')

    from . import MPI
    comm = MPI.COMM_WORLD
    if not args:
        if comm.rank == 0:
            parser.print_usage()
        parser.exit()
    command = args.pop(0)
    if command not in main.commands:
        if comm.rank == 0:
            parser.error("unknown command '%s'" % command)
        parser.exit(2)
    command = main.commands[command]
    command(comm, args=args)
    parser.exit()

main.commands = {
    'helloworld': helloworld,
    'ringtest': ringtest,
}

if __name__ == '__main__':
    main()
