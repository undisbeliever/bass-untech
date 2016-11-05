include ../nall/GNUmakefile

name := bass
flags += -I.. -O3
link +=
objects := obj/bass.o

all: $(objects)
	$(compiler) -o out/$(name) $(objects) $(link)

obj/bass.o: bass.cpp bass.hpp $(call rwildcard,core/) $(call rwildcard,arch/)
	$(compiler) $(cppflags) $(flags) -o obj/bass.o -c bass.cpp

clean:
	$(call delete,obj/*)
	$(call delete,out/*)

install:
	if [ -f out/$(name) ]; then cp out/$(name) $(prefix)/bin/$(name); fi

uninstall:
	if [ -f $(prefix)/bin/$(name) ]; then rm $(prefix)/bin/$(name); fi
