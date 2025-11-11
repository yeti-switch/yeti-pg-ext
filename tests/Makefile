# apt install postgresql-16-pgtap libtap-parser-sourcehandler-pgtap-perl
# https://pgtap.org/documentation.html

D:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
all:
	pg_prove -v -h 127.0.0.1 -U yeti $D/*.sql

%:
	pg_prove -v -h 127.0.0.1 -U yeti $@.sql
