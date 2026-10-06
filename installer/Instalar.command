#!/bin/bash
# Instala Voxora en las carpetas de plugins del usuario y quita el bloqueo de macOS.
cd "$(dirname "$0")"

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

rm -rf "$VST3_DIR/Voxora.vst3" "$AU_DIR/Voxora.component"
cp -R Voxora.vst3 "$VST3_DIR/"
cp -R Voxora.component "$AU_DIR/"

xattr -cr "$VST3_DIR/Voxora.vst3" "$AU_DIR/Voxora.component"
codesign --force --deep -s - "$VST3_DIR/Voxora.vst3" "$AU_DIR/Voxora.component" 2>/dev/null

echo ""
echo "Voxora instalado. Abre FL Studio > Options > Manage plugins > Find installed plugins."
echo "Puedes cerrar esta ventana."
