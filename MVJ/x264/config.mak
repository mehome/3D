SRCPATH=.
prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=X86
SYS=WINDOWS
CC=gcc
CFLAGS=-Wshadow -O3 -ffast-math  -Wall -I. -I$(SRCPATH) -march=i686 -mfpmath=sse -msse -std=gnu99 -fomit-frame-pointer -fno-tree-vectorize -fno-zero-initialized-in-bss
DEPMM=-MM -g0
DEPMT=-MT
LD=gcc -o 
LDFLAGS= -Wl,--large-address-aware
LIBX264=libx264.a
AR=ar rc 
RANLIB=ranlib
STRIP=strip
AS=yasm
ASFLAGS= -O2 -f win32 -DPREFIX -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8
RC=windres -I. -o 
EXE=.exe
HAVE_GETOPT_LONG=1
DEVNULL=NUL
PROF_GEN_CC=-fprofile-generate
PROF_GEN_LD=-fprofile-generate
PROF_USE_CC=-fprofile-use
PROF_USE_LD=-fprofile-use
default: cli
install: install-cli
LDFLAGSCLI = 
CLI_LIBX264 = $(LIBX264)
