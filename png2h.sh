#!/bin/bash
convert $1 -rotate 90 pnm:- | tail -n +3 | xxd -i > $(basename $1 .png).h
