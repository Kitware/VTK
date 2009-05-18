# Makefile for MPI stubs - edit this for your platform

SHELL = /bin/sh
.IGNORE:

# Files

SRC =		mpi.c
INC =		mpi.h

# Definitions

EXE =		libmpi.a
OBJ = 		$(SRC:.c=.o)

# System-specific settings

CC =		g++
CCFLAGS =	-O
ARCHIVE =	ar
ARCHFLAG =	rs

# Target

$(EXE):	$(OBJ)
	$(ARCHIVE) $(ARCHFLAG) $(EXE) $(OBJ)

# Clean

clean:
	rm *.o libmpi.a

# Compilation rules

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

# Individual dependencies

$(OBJ):	$(INC)
