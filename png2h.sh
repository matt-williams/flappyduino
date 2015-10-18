#!/bin/bash
convert $1 -background white -alpha remove -rotate 90 -monochrome pnm:- | tail -n +3 | xxd -i > $(basename $1 .png).h
convert $1 -alpha extract -negate -monochrome -rotate 90 pnm:- | tail -n +3 | xxd -i > $(basename $1 .png).mask.h
