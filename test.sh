#!/bin/bash
./radar_server &
PID=$!
sleep 1
OUT=$(timeout 3 bash -c 'exec 3<>/dev/tcp/127.0.0.1/5050; while read -r line <&3; do echo "$line"; done')
kill $PID
if echo "$OUT" | grep -q "^SCAN 1$" && echo "$OUT" | grep -q "^END$"; then
    echo "TEST OK"
else
    echo "TEST FALLITO"
    exit 1
fi