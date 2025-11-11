# apt install postgresql-18-pgtap libtap-parser-sourcehandler-pgtap-perl
# https://pgtap.org/documentation.html

#pg_createcluster -p 5454 18 test --start -- --auth=trust
#psql -p5454 -U postgres  -c 'create user yeti superuser;' -c 'create database yeti owner yeti;'
#psql -p5454 -U yeti -c 'create schema yeti_ext;' -c 'create extension yeti with schema yeti_ext;'
#pg_dropcluster 18 test --stop

D:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
all:
	pg_prove -v -p 5454 -U yeti $D/*.sql

%:
	pg_prove -v -p 5454 -U yeti $@.sql
