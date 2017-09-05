########################################################################
# Code listing from "Advanced Linux Programming," by CodeSourcery LLC  #
# Copyright (C) 2001 by New Riders Publishing                          #
# See COPYRIGHT for license information.                               #
########################################################################

PROGRAMS = getopt_long
CFLAGS   =

.PHONY:         all clean

all:            $(PROGRAMS)

debug: CFLAGS += -g3 -DDEBUG
debug: $(PROGRAMS)

# Clean up build products.
clean:
	rm -f *.o *.a $(PROGRAMS)

getopt_long: getopt_long.c readfile.c
	$(CC) $(CFLAGS) -o $(PROGRAMS) getopt_long.c readfile.c


