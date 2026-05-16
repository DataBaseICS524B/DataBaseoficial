#!/bin/bash
DEST="/mnt/c/CustomDB_GUI"
SRC="$HOME/DataBaseoficial/src/gui"

echo "Copying GUI to $DEST ..."
rm -rf "$DEST"
mkdir -p "$DEST"

# Копируем, исключая bin/obj
cp -r "$SRC/." "$DEST/" 2>/dev/null
rm -rf "$DEST/bin" "$DEST/obj" 2>/dev/null

echo "Starting GUI on Windows..."
cmd.exe /c "start cmd.exe /k \"cd /d C:\\CustomDB_GUI && dotnet run\""