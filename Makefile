# Top-level Makefile for nabla -- convenience targets that dispatch to the
# per-directory Makefiles.
#
#   make            build both nabla (32-bit) and nabla64 (64-bit) into
#                   cli/build/
#   make nabla      build the default 32-bit variant only
#   make nabla64    build the 64-bit variant only
#   make test       build and run the test suite in both widths
#   make clean      remove the build directories
#
# The real build logic lives in cli/Makefile and Test/Makefile.  The 64-bit
# variant is compiled in its own build64/ object directory (its -DLENGTH_SIZE_64
# objects must not collide with the 32-bit ones), then copied into cli/build/
# alongside nabla.

.PHONY: all nabla nabla64 test clean

all: nabla nabla64

nabla:
	$(MAKE) -C cli

nabla64:
	$(MAKE) -C cli LENGTH_SIZE=64 BUILD=build64
	mkdir -p cli/build
	cp cli/build64/nabla64 cli/build/nabla64

test:
	$(MAKE) -C Test test
	$(MAKE) -C Test LENGTH_SIZE=64 BUILD=build64 test

clean:
	$(MAKE) -C cli clean
	$(MAKE) -C cli BUILD=build64 clean
	$(MAKE) -C Test clean
	$(MAKE) -C Test BUILD=build64 clean
