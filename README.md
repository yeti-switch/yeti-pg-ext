[![Build Status](https://github.com/yeti-switch/yeti-pg-ext/actions/workflows/main.yml/badge.svg)](https://github.com/yeti-switch/yeti-pg-ext/actions/workflows/main.yml)
[![Made in Ukraine](https://img.shields.io/badge/made_in-ukraine-ffd700.svg?labelColor=0057b7)](https://stand-with-ukraine.pp.ua)

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-team.svg)](https://stand-with-ukraine.pp.ua)

# yeti-pg-ext

yeti-pg-ext is a part of project [Yeti]

## Build & Installation

### Debian

## Installation via Package (Debian 12/bookworm)
```sh
# apt install wget gnupg

# echo "deb [arch=amd64] http://apt.postgresql.org/pub/repos/apt bullseye-pgdg main" > /etc/apt/sources.list.d/pgdg.list
# wget -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | gpg --dearmor > /etc/apt/trusted.gpg.d/apt.postgresql.org.gpg

# echo "deb [arch=amd64] https://deb.yeti-switch.org/debian/1.13 bookworm main" > /etc/apt/sources.list.d/yeti.list
# wget http://deb.yeti-switch.org/yeti.gpg -O /etc/apt/trusted.gpg.d/deb.yeti-switch.org.asc

# apt update
# apt install postgresql-16-yeti
```

## Building from sources (Debian 12+)

#### get sources

```sh
$ git clone https://github.com/yeti-switch/yeti-pg-ext.git
$ cd yeti-pg-ext
```

#### install build dependencies

```sh
# echo "deb [arch=amd64] http://apt.postgresql.org/pub/repos/apt bullseye-pgdg main" > /etc/apt/sources.list.d/pgdg.list
# wget -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | gpg --dearmor > /etc/apt/trusted.gpg.d/apt.postgresql.org.gpg
# apt update
# apt build-dep .
```

### build package
```sh
$ make deb
```

## Building for Darwin (for web interface testing purposes only)

### install build dependencies

```sh
# brew install postgresql nanomsg
```

### get sources & build & install

```sh
$ git clone https://github.com/yeti-switch/yeti-pg-ext.git
$ cd yeti-pg-ext
$ make
# make install
```

## add extension in postgresql

### create extension

```sql
yeti=# CREATE SCHEMA yeti_ext;
CREATE SCHEMA

yeti=# CREATE EXTENSION yeti WITH SCHEMA yeti_ext;
CREATE EXTENSION

yeti=# \dx yeti
                 List of installed extensions
 Name | Version |  Schema  |            Description
------+---------+----------+-----------------------------------
 yeti | 1.3.9   | yeti_ext | helper functions for YETI project
(1 row)

yeti=# \df yeti_ext.*
                                                                                     List of functions
  Schema  |             Name              |        Result data type        |                                          Argument data types                                          |  Type
----------+-------------------------------+--------------------------------+-------------------------------------------------------------------------------------------------------+--------
 yeti_ext | lnp_endpoints_cache_set       | void                           | key text, response text, error boolean                                                                | func
 yeti_ext | lnp_endpoints_set             | void                           | endpoints text[]                                                                                      | func
 yeti_ext | lnp_endpoints_show            | SETOF yeti_ext.endpoints       |                                                                                                       | func
 yeti_ext | lnp_resolve_cnam              | json                           | database_id integer, data json                                                                        | func
 yeti_ext | lnp_resolve_tagged            | yeti_ext.lnp_result            | database_id integer, local_number text                                                                | func
 yeti_ext | lnp_resolve_tagged_with_error | yeti_ext.lnp_result_with_error | database_id integer, local_number text                                                                | func
 yeti_ext | lnp_set_rtt_timeout           | void                           | timeout_msec integer                                                                                  | func
 yeti_ext | lnp_set_timeout               | void                           | timeout_msec integer                                                                                  | func
 yeti_ext | rank_dns_srv                  | integer                        | weight integer                                                                                        | window
 yeti_ext | regexp_replace_rand           | text                           | text_in text, regexp_rule text, regexp_result text, keep_empty boolean DEFAULT false                  | func
 yeti_ext | regexp_replace_rand           | text[]                         | text_in text[], regexp_rule text, regexp_result text, keep_empty boolean DEFAULT false                | func
 yeti_ext | regexp_replace_rand           | text                           | text_in text, regexp_rule text, regexp_result text, regexp_opt text, keep_empty boolean DEFAULT false | func
 yeti_ext | replace_rand                  | text                           | text                                                                                                  | func
 yeti_ext | tag_action                    | integer[]                      | op integer, a integer[], b integer[]                                                                  | func
 yeti_ext | tag_compare                   | integer                        | a integer[], b integer[], match_mode smallint DEFAULT 0                                               | func
 yeti_ext | tbf_rate_check                | boolean                        | namespace_id integer, bucket_id bigint, rate real                                                     | func
(16 rows)
```

### add extension to the `shared_preload_libraries` to use tbf_rate_check() function (needs postgresql restart)
```sh
# grep yeti /etc/postgresql/16/main/postgresql.conf
shared_preload_libraries='yeti_pg_ext'

# pg_ctlcluster 16 main restart
```

[Yeti]:http://yeti-switch.org/
