ENABLE_HOOK:=yes


include build.mk

.ONESHELL:
.SUFFIXES:

GLSLC:=glslangValidator

VKCUBE_BINARY:=vkcube
VKCUBE_SRC:=cube.c esTransform.c main.c
VKCUBE_PKGCONFIG_DEPS:=xcb

SHADERS:=vkcube.vert vkcube.frag
SHADER_SPVS:=$(SHADERS:%=%.spv)
SHADER_HEADERS:=$(SHADERS:%=%.spv.h)


HOOK_LIBRARY:=hook.so
HOOK_SRC:=hook.c vkhelper.c

BLIT_SHADERS:=blit.vert blit.frag
BLIT_SHADER_SPVS:=$(BLIT_SHADERS:%=%.spv)
BLIT_SHADER_SOURCES:=$(BLIT_SHADERS:%=%.c)
BLIT_SHADER_OBJECTS:=$(BLIT_SHADERS:%=%.o)

#
# To run vkcube with hook and sanitizer, use below command
#
# $ LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.3:./hook.so ./vkcube
#

SANITIZER_FLAGS:=-fsanitize=address

DEBUG_FLAGS:=-g $(SANITIZER_FLAGS)


all: $(VKCUBE_BINARY) $(HOOK_LIBRARY)

clean: $(VKCUBE_BINARY)_clean $(HOOK_LIBRARY)_clean
	rm -f $(SHADER_HEADERS) $(SHADER_SPVS) $(BLIT_SHADER_SOURCES) $(BLIT_SHADER_SPVS)

$(VKCUBE_BINARY)_cflags:=-I./ -Wall $(DEBUG_FLAGS)
$(VKCUBE_BINARY)_ldflags:=$(shell pkg-config --libs $(VKCUBE_PKGCONFIG_DEPS)) -lvulkan -lm $(DEBUG_FLAGS)
$(eval $(call define_c_target,$(VKCUBE_BINARY),$(VKCUBE_SRC)))

$(HOOK_LIBRARY)_cflags:=-I./ -Wall -fPIC $(DEBUG_FLAGS)
$(HOOK_LIBRARY)_ldflags:=-lvulkan -shared $(DEBUG_FLAGS)
$(eval $(call define_c_target,$(HOOK_LIBRARY),$(HOOK_SRC) $(BLIT_SHADER_SOURCES)))


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

