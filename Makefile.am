ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib/libdynamic lib/libclo lib/libreactor

bin_PROGRAMS = echo_server
echo_server_SOURCES = src/echo_server.c
echo_server_CFLAGS = -std=gnu11 -O3 -flto -fuse-linker-plugin
echo_server_LDADD = -lreactor -ldynamic -lclo

MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in
maintainer-clean-local:; rm -rf autotools m4
