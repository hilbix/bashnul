#!/bin/bash

time for a in /usr/bin/*; do [ -f "$a" ] && [ -r "$a" ] || continue; LC_ALL=C ./bashnul -e <"$a" | LC_ALL=C ./bashnul -d | cmp - "$a" || break; done

