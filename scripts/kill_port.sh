#!/bin/bash
# scripts/kill_port.sh

PORT=${1:-5432}

echo "Killing processes on port $PORT..."

if command -v fuser &> /dev/null; then
    sudo fuser -k $PORT/tcp
elif command -v lsof &> /dev/null; then
    sudo kill -9 $(sudo lsof -t -i:$PORT)
else
    echo "No tool found to kill processes on port $PORT"
fi

echo "Port $PORT should be free now"