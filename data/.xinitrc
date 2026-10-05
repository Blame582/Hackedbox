#!/bin/sh

if [ -n "$1" ]; then
    exec "$1"
fi

exec /usr/bin/hackedbox-session