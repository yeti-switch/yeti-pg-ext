[![Build Status](https://github.com/yeti-switch/yeti-pg-ext/actions/workflows/tests.yml/badge.svg)](https://github.com/yeti-switch/yeti-pg-ext/actions/workflows/tests.yml)
[![Made in Ukraine](https://img.shields.io/badge/made_in-ukraine-ffd700.svg?labelColor=0057b7)](https://stand-with-ukraine.pp.ua)

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-team.svg)](https://stand-with-ukraine.pp.ua)

# yeti-pg-ext

yeti-pg-ext is a part of project [Yeti]

## Build & Installation

### Debian

#### install build dependencies

```sh
# aptitude install libnanomsg-dev postgresql-server-dev-all dh-make devscripts pkg-config
```

#### get sources & build package

```sh
$ git clone git@github.com:yeti-switch/yeti-pg-ext.git
$ cd yeti-pg-ext
$ make deb
```

### Darwin (for web interface testing purposes only)

#### install build dependencies

```sh
# brew install postgresql nanomsg
```

#### get sources & build & install

```sh
$ git clone git@github.com:yeti-switch/yeti-pg-ext.git
$ cd yeti-pg-ext
$ make
# make install
```


[Yeti]:http://yeti-switch.org/
