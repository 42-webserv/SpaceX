#!/bin/bash
while true
do
sleep 1
leaks $(ps | grep spacex | grep -v grep | head -n 1 | awk '{ print $1 }')
done