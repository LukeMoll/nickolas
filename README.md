Nickolas - NickServ nickname rewnewal
===

Nickolas will login to Freenode and change its nickname to each of the ones associated with your NickServ account, to ensure they are not dropped.

## Requirements
In order to build, you will need the following packages on Linux:

- `cmake`
- `libircclient-dev`
- `libpcre3-dev`

Build with `./make.sh`.

## Usage
```
./nickolas yournickname "YourNickServPassword"
```

Optionally add `-v` or `--verbose` to show additional information.

## Contribution
Contribution is welcome, this project was essentially me teaching myself C, so the quality is what you'd expect.