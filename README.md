# Mediakey

A simple program that emulates headset media keys.

## Dependencies

- GLib 2.0
- Playerctl

## Building

```sh
gcc mediakey.c -Ofast -o mediakey -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lplayerctl
```
