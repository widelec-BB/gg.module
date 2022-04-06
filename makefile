# makefile for GNU make (automatically generated by makemake 14-Kwi-13, 11:49:08)
# NOTE: indent with TAB in GNU makefiles!

# MESSAGES #

COMPILE_FILE=printf "\033[K\033[0;33mCompiling \033[1;33m$<\033[0;33m...\033[0m\n"
TARGET_DONE=printf "\033[K\033[0;32mTarget \"$@\" successfully done.\033[0m\n"
LINKING=printf "\033[K\033[1;34mLinking project \"$@\"... \033[0m\n"
NOTMORPHOS=printf "\033[K\033[1;91mTarget \"$@\" is not supported in non-MorphOS enviroment. \033[0m\n"

# VARIABLES #
OS = $(shell uname -s)

NIL = /dev/null 2>&1
ifeq ($(OS),MorphOS)
	NIL = NIL:
endif

# PROJECT #
# paths are relative to the project directory (current directory during make)

OUTFILE = gg.module

OUTDIR  = bin/
OBJDIR  = o/

PROJECT = $(OUTDIR)$(OUTFILE)

# path to directory containing KwaKwa source code
KWAKWAAPI = ../kwakwa/

# COMPILER #
CC = ppc-morphos-gcc-11
CWARNS = -Wall -Wno-pointer-sign
CDEFS  = -DUSE_INLINE_STDARG -D__NOLIBBASE__ -D__AMIGADATE__=\"\ \($(shell date "+%d.%m.%Y")\)\ \" -DAROS_ALMOST_COMPATIBLE
# -D__DEBUG__
CFLAGS = -O3 -noixemul
CLIBS  = -I$(KWAKWAAPI) -Igglib

COMPILE = $(CC) $(TARGET) $(CWARNS) $(CDEFS) $(CFLAGS) $(CLIBS)

# LINKER #
LD = ppc-morphos-gcc-11

LWARNS =
LDEFS  =
LFLAGS = -noixemul -nostdlib -nostartfiles
LIBS   = -Lgglib/ -labox -lgg -lvstring -lssl_shared -lcrypto_shared
# -ldebug

LINK   = $(LD) $(TARGET) $(LWARNS) $(LDEFS) $(LFLAGS)

# target 'all' (default target)
all: gglib/libgg.a $(PROJECT)
	@$(TARGET_DONE)

# depenedncies targets
gglib/libgg.a: gglib/*.c gglib/*.h
	@make -C gglib

# target 'compiler' (compile target)
$(OBJDIR)class.c.o: class.c class.h globaldefines.h translations.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)class.c.o class.c

$(OBJDIR)lib.c.o: lib.c class.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)lib.c.o lib.c

$(OBJDIR)events.c.o: events.c events.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)events.c.o events.c

$(OBJDIR)gui.c.o: gui.c globaldefines.h gui.h locale.h translations.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)gui.c.o gui.c

$(OBJDIR)multilogonlist.c.o: multilogonlist.c globaldefines.h multilogonlist.h locale.h translations.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)multilogonlist.c.o multilogonlist.c

$(OBJDIR)locale.c.o: locale.c locale.h translations.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)locale.c.o locale.c

$(OBJDIR)support.c.o: support.c globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)support.c.o support.c

OBJS = $(OBJDIR)lib.c.o $(OBJDIR)class.c.o $(OBJDIR)events.c.o $(OBJDIR)gui.c.o $(OBJDIR)locale.c.o $(OBJDIR)multilogonlist.c.o\
 $(OBJDIR)support.c.o

# link all file(s)
$(PROJECT): $(OBJS) gglib/libgg.a
	@$(LINKING)
	@$(LINK) $(OBJS) -o $(PROJECT) $(LIBS)

# any other targets
.PHONY: strip clean dump dist install

translations.h: locale/$(OUTFILE).cs
ifeq ($(OS),MorphOS)
	MakeDir ALL $(OUTDIR)catalogs/polski
	SimpleCat locale/$(OUTFILE).cs
else
	@$(NOTMORPHOS)
endif

strip:
	@strip --strip-unneeded --remove-section=.comment $(PROJECT)
	@$(TARGET_DONE)

clean:
ifeq ($(OS),MorphOS)
	@-rm -rf translations.h $(OUTDIR)catalogs
endif
	@make -C gglib clean >$(NIL)
	@-rm $(PROJECT) >$(NIL)
	@-rm $(OBJDIR)*.o >$(NIL)
	@$(TARGET_DONE)

dump:
ifeq ($(OS),MorphOS)
	@objdump -dC $(OUTDIR)/$(OUTFILE) >RAM:$(OUTFILE).txt
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif

dist: all
ifeq ($(OS),MorphOS)
	@-rm -rf RAM:$(OUTFILE) RAM:$(OUTFILE).lha
	@mkdir RAM:$(OUTFILE) >NIL:
	@mkdir RAM:$(OUTFILE)/modules >NIL:
	@copy $(OUTDIR)$(OUTFILE) RAM:$(OUTFILE)/modules/$(OUTFILE) >NIL:
	@copy $(OUTDIR)catalogs RAM:$(OUTFILE)/catalogs ALL >NIL:
	@copy doc RAM:$(OUTFILE) ALL >NIL:
	@copy LICENSE RAM:$(OUTFILE) >NIL:
	@strip --strip-unneeded --remove-section .comment RAM:$(OUTFILE)/modules/$(OUTFILE) >NIL:
	@find RAM:$(OUTFILE) \( -name .svn -o -name .git \) -printf "\"%p\"\n" | xargs rm -rf
	@MOSSYS:C/LHa a -r -a RAM:$(OUTFILE).lha RAM:$(OUTFILE)/ >NIL:
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif

install:
ifeq ($(OS),MorphOS)
	@copy $(OUTDIR)$($OUTFILE) SYS:Applications/KwaKwa/modules/ >NIL:
	@copy ALL bin/catalogs/ SYS:Applications/KwaKwa/catalogs/ >NIL:
	@avail flush
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif
