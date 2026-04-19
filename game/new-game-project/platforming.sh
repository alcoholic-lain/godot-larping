#!/bin/sh
printf '\033c\033]0;%s\a' New Game Project
base_path="$(dirname "$(realpath "$0")")"
"$base_path/platforming.x86_64" "$@"
