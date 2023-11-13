# We first define a few make macros (symbolic constants) with "=".
# Like macros in general, make macros help us maintain "D-R-Y" (Don't
# Repeat Yourself) and make the makefile more readable.

# ASSIGNMENT
#
# Fill in the {}'s with the approriate file names. These should be the
# only changes you need to make to the makefile.

# These are the executables (aka "binfiles" or "bins").
BINS = whocan

# "-g" says to compile in support for the debugger (gdb). The "-W"
# warnings are the bare minimum I recommend. There are lots of others:
# see "info gcc".
CFLAGS = -g -Wall -Wstrict-prototypes `pkg-config --cflags glib-2.0` -O0

# "-g" says to link in support for the debugger (gdb).
LDFLAGS = -g `pkg-config --libs glib-2.0`

# Having defined macros, we start on the actual targets.

# ".PHONY" declares a target that is not a real file. The first target
# is the default target produced by entering "make" alone on the
# command line, hence the name "default". Apart from this, I generally
# put targets in alphabetical order for readability.
.PHONY: default
default: $(BINS)

# For each object file, create a rule like this that says (a) what it
# depends on (to the right of ":") and (b) how to compile the source
# file into the object file. "$<" means "the first file (here the
# source file) to the right of the ':'" above.
whocan.o: whocan.c
	$(CC) $(CFLAGS) -c $< -o $@

# This target gets rid of all the "*.o" files that otherwise clutter
# the directory, but leaves the executable intact.
.PHONY: clean
clean:
	rm -f core* *.o *~

# This says how to link one or more "*.o" files into a bin. "$^" means
# "all of the files to the right of the ':'". "$@" means the "target"
# (what's on the left of the ':'"). Note that $(CC) is used both for
# compiling and loading.
#
# You will need one rule like this for each bin.
whocan: whocan.o
	$(CC) $^ -o $@ $(LDFLAGS)

test: whocan
	-./whocan cd /dev/null
	@echo
	-./whocan read whocan.c
	@echo
	-./whocan execute whocan
	@echo
	-./whocan search .
	@echo
	-./whocan cd /root
	-./whocan cd /home/edna
	@echo
	-./whocan ls whocan
	-./whocan ls /home/lost+found

# The "immaculate" target is a more extreme version of "clean",
# deleting all of the files that can be reconstructed automatically
# from source. What's left over after "make immaculate" should be
# what you submit in your tarball.
.PHONY: immaculate
immaculate: clean
	rm -f $(BINS)

