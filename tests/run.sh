#!/bin/bash
set -e
for f in *.sql; do
	psql -a $@ -v ON_ERROR_STOP=1 -f $f
done

