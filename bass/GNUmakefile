build := optimize
console := true
include ../nall/GNUmakefile

flags += -I..

name := bass-untech
objects := obj/bass.o
arch_dir := bass/architectures

ifeq ($(platform), windows)
	name := $(name).exe
endif

all: out/$(name) out-architectures

out/$(name): out $(objects)
	$(compiler) -o out/$(name) $(objects) $(link)

obj/bass.o: obj bass.cpp bass.hpp $(call rwildcard,core/) $(call rwildcard,architecture/)
	$(compiler) $(cppflags) $(flags) -o obj/bass.o -c bass.cpp

clean:
	-@$(call delete,obj/*)
	-@$(call delete,out/$(name))
	-@$(call delete,out/architectures/*)

obj out:
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


# Copy architectures to out/ so bass-untech can be used without installing it
architectures := $(wildcard data/architectures/*.arch)

.PHONY: out-architectures
out-architectures: out/architectures
out-architectures: $(architectures:data/architectures/%=out/architectures/%)

out/architectures/%.arch: data/architectures/%.arch
ifeq ($(platform), windows)
	copy "$(subst /,\,$<)" "$(subst /,\,$@)"
else
	cp "$<" "$@"
endif

out/architectures:
ifeq ($(platform), windows)
	mkdir out\architectures
else
	mkdir -p out/architectures
endif

