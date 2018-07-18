#!/bin/bash -
clang -std=c11 -Wall -Wextra -pedantic -lircclient -lpcre -o nickolas nickolas.c linkedlist.c
