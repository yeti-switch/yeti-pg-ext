FROM debian:bullseye-slim

ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8

ADD https://www.postgresql.org/media/keys/ACCC4CF8.asc /etc/apt/trusted.gpg.d/postgres.asc

RUN echo "deb http://apt.postgresql.org/pub/repos/apt/ bullseye-pgdg main" >> /etc/apt/sources.list  && \
    chmod 644 /etc/apt/trusted.gpg.d/*.asc

RUN apt update && \
    apt -y --no-install-recommends install postgresql-common

COPY debian/control.in debian/control.in
COPY debian/pgversions debian/pgversions

RUN pg_buildext updatecontrol

RUN apt update && \
    apt -y --no-install-recommends build-dep . && \
    rm -r debian/

