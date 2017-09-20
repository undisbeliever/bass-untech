build := release
include ../nall/GNUmakefile

flags += -I..

name := bass-untech
objects := obj/bass.o
arch_dir := bass/architectures

all: out/$(name)

out/$(name): out/ $(objects)
	$(compiler) -o out/$(name) $(objects) $(link)

obj/bass.o: obj/ bass.cpp bass.hpp $(call rwildcard,core/) $(call rwildcard,architecture/)
	$(compiler) $(cppflags) $(flags) -o obj/bass.o -c bass.cpp

clean:
	-@$(call delete,obj/*)
	-@$(call delete,out/*)

obj/ out/:
	mkdir $@

install:
ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifneq ($(filter $(platform),linux bsd),)
	if [ -f out/$(name) ]; then cp out/$(name) $(prefix)/bin/$(name); fi
	mkdir -p $(prefix)/share/$(arch_dir)/
	cp -R data/architectures/* $(prefix)/share/$(arch_dir)/
endif

uninstall:
ifeq ($(shell id -un),root)
	$(error "make uninstall should not be run as root")
else ifneq ($(filter $(platform),linux bsd),)
	if [ -f $(prefix)/bin/$(name) ]; then rm $(prefix)/bin/$(name); fi
endif
