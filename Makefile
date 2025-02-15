TARGET_EXEC:=smw

SRCS:=$(wildcard smb1/*.c smbll/*.c src/*.c src/snes/*.c) third_party/gl_core/gl_core_3_1.c third_party/sds/sds.c
OBJS:=$(SRCS:%.c=%.o)

PYTHON:=/usr/bin/env python3
CFLAGS:=$(if $(CFLAGS),$(CFLAGS),-O2 -fno-strict-aliasing -Werror )
CFLAGS:=${CFLAGS} $(shell sdl2-config --cflags) -DSYSTEM_VOLUME_MIXER_AVAILABLE=0 -Wno-tautological-constant-out-of-range-compare -I/opt/homebrew/Cellar/lua/5.4.6/include/lua/ -I.
LDFLAGS = -llua

ifeq (${OS},Windows_NT)
    WINDRES:=windres
#    RES:=sm.res
    SDLFLAGS:=-Wl,-Bstatic $(shell sdl2-config --static-libs)
else
    SDLFLAGS:=$(shell sdl2-config --libs) -lm
endif

.PHONY: all clean clean_obj

all: $(TARGET_EXEC) smw_assets.dat
$(TARGET_EXEC): $(OBJS) $(RES)
	$(CC) -g $^ -o $@ $(LDFLAGS) $(SDLFLAGS)

# debug: CFLAGS += -DDEBUG=1
debug: all


%.o : %.c
	$(CC) -g -c $(CFLAGS) $< -o $@

#$(RES): src/platform/win32/smw.rc
#	@echo "Generating Windows resources"
#	@$(WINDRES) $< -O coff -o $@

smw_assets.dat:
	@echo "Extracting game resources"
	$(PYTHON) assets/restool.py

clean: clean_obj clean_gen
clean_obj:
	@$(RM) $(OBJS) $(TARGET_EXEC)
clean_gen:
	@$(RM) $(RES) smw_assets.dat
	@rm -rf assets/__pycache__
