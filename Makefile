MODULE_big = yeti_pg_ext
EXTENSION = yeti

SRSC = $(wildcard *.c src/*.c)
OBJS = $(SRSC:.c=.o)

DATA = $(wildcard *.sql)

PGVER = $(shell echo $(VERSION) | awk -F. '{ print $$1*100+$$2 }')

PG_CPPFLAGS  = -DPGVER=$(PGVER)

SHLIB_LINK = $(shell pkg-config nanomsg --libs)
PG_CFLAGS = $(shell pkg-config --cflags $(NANOMSG))

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)

deb: clean
	pg_buildext updatecontrol
	make -f debian/rules debian/control
	dh clean
	debuild -us -uc -b

