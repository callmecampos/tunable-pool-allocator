## Process with automake --> Makefile.in

TESTS = check_pool_alloc
check_PROGRAMS = check_pool_alloc
check_pool_alloc_SOURCES = check_pool_alloc.c %(top_builddir)/src/pool_alloc.h
check_pool_alloc_CFLAGS = @CHECK_CFLAGS@
check_pool_alloc_LDADD = $(top_builddir)/src/libpoolalloc.la @CHECK_LIBS@