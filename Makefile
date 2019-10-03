ENABLE_HOOK:=yes


include build.mk

.ONESHELL:
.SUFFIXES:

GLSLC:=glslangValidator

BINARY:=vkcube
SRC:=cube.c esTransform.c main.c vkhelper.c hook.c
PKGCONFIG_DEPS:=libpng xcb

SHADERS:=vkcube.vert vkcube.frag
SHADER_SPVS:=$(SHADERS:%=%.spv)
SHADER_HEADERS:=$(SHADERS:%=%.spv.h)

BLIT_SHADERS:=blit.vert blit.frag
BLIT_SHADER_SPVS:=$(BLIT_SHADERS:%=%.spv)
BLIT_SHADER_SOURCES:=$(BLIT_SHADERS:%=%.c)
BLIT_SHADER_OBJECTS:=$(BLIT_SHADERS:%=%.o)

ifeq ($(ENABLE_HOOK),yes)
HOOK_FLAG:=-DENABLE_HOOK
else
HOOK_FLAG:=
endif

SANITIZER_FLAGS:=-fsanitize=address
DEBUG_FLAGS:=-g $(SANITIZER_FLAGS)

all: $(BINARY)

clean:
	rm -f *.o $(BINARY) $(SHADER_HEADERS) $(SHADER_SPVS) $(BLIT_SHADER_SOURCES) $(BLIT_SHADER_SPVS)

$(BINARY)_cflags:=-I./ -Wall $(DEBUG_FLAGS) $(HOOK_FLAG)
$(BINARY)_ldflags:=$(shell pkg-config --libs $(PKGCONFIG_DEPS)) -lvulkan -lm $(DEBUG_FLAGS)
$(eval $(call define_c_target,$(BINARY),$(SRC) $(BLIT_SHADER_SOURCES)))

$(SHADER_SPVS) $(BLIT_SHADER_SPVS): %.spv: %
	@echo "\tGLSLC\t$@"
	$(GLSLC) -V $< -o $@ >/dev/null

$(BLIT_SHADER_SOURCES): %.c: %.spv
	@echo "\tXXD\t$@"
	xxd -i $< > $@

$(SHADER_HEADERS): %.spv.h: %.spv
	@echo "\tHEADER\t$@"
	cat $< | hexdump -v -e '/4 "0x%08X, " ""' |fold -48 > $@


cube.o: cube.c $(SHADER_HEADERS)

