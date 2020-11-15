## Process with automake --> Makefile.in

TESTS = check_pool_alloc runtime_pool_alloc runtime_pool_init
check_PROGRAMS = check_pool_alloc runtime_pool_alloc runtime_pool_init
check_pool_alloc_SOURCES = check_pool_alloc.c %(top_builddir)/src/pool_alloc.h
check_pool_alloc_CFLAGS = @CHECK_CFLAGS@
check_pool_alloc_LDADD = $(top_builddir)/src/libpoolalloc.la @CHECK_LIBS@

runtime_pool_alloc_SOURCES = runtime_pool_alloc.c %(top_builddir)/src/pool_alloc.h
runtime_pool_alloc_CFLAGS = @CHECK_CFLAGS@
runtime_pool_alloc_LDADD = $(top_builddir)/src/libpoolalloc.la @CHECK_LIBS@

runtime_pool_init_SOURCES = runtime_pool_init.c %(top_builddir)/src/pool_alloc.h
runtime_pool_init_CFLAGS = @CHECK_CFLAGS@
runtime_pool_init_LDADD = $(top_builddir)/src/libpoolalloc.la @CHECK_LIBS@