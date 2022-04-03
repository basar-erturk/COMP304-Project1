#! /bin/sh

text=$(curl -s  https://icanhazdadjoke.com)

notify-send "For some reason curl does not work with crontab execution, but works when written in terminal."

echo "control"

echo "\n"

echo "$text"

