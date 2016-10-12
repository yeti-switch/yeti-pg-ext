#!/bin/bash
for f in *.sql; do
	psql -a $@ < $f
done

