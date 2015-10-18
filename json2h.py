#!/usr/bin/python

import json
import sys

tileMap = json.load(sys.stdin)
data = tileMap['layers'][0]['data']
data = [x - 1 for x in data]
data = [data[y] for x in range(0, len(data) / 6) for y in range(x, len(data), len(data) / 6)]
print ', '.join(map(str, data))
