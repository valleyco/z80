#!/usr/bin/env bash
# Download Kaypro 4/84 Universal ROM + CP/M disk from retroarchive.org,
# unzip toolkit archives, and convert TeleDisk .td0 to raw .dsk.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS="$ROOT/kaypro/assets"
TD0_TOOL="$ROOT/tools/td0_to_dsk/td0_to_dsk"
BASE="http://www.retroarchive.org/maslin"

# Kaypro 4/84 DSDD: 40 tracks, 2 sides, 9 sectors, 512 bytes
KAYPRO_DSDD_SIZE=$((40 * 2 * 9 * 512))

log() { printf '==> %s\n' "$*"; }

download() {
  local url="$1"
  local dest="$2"
  if [[ -f "$dest" ]]; then
    log "already have $(basename "$dest"), skipping download"
    return 0
  fi
  log "downloading $(basename "$dest")"
  curl -fsSL --retry 3 --retry-delay 2 -o "$dest" "$url"
}

mkdir -p "$ASSETS"

log "Kaypro assets -> $ASSETS"

download "$BASE/roms/kaypro/81-478a.rom" "$ASSETS/81-478a.rom"

download "$BASE/disks/kaypro/kaypro1.td0" "$ASSETS/kaypro1.td0"

for zip in urmtkit1 kp4diag k10udiag; do
  download "$BASE/disks/kaypro/${zip}.zip" "$ASSETS/${zip}.zip"
  log "unzipping ${zip}.zip"
  mkdir -p "$ASSETS/${zip}"
  unzip -o -q "$ASSETS/${zip}.zip" -d "$ASSETS/${zip}"
done

build_td0_tool() {
  if [[ -x "$TD0_TOOL" ]]; then
    return 0
  fi
  log "building td0_to_dsk (TeleDisk -> Kaypro .dsk)"
  make -C "$ROOT/tools/td0_to_dsk" >/dev/null
}

build_td0_tool
log "converting kaypro1.td0 -> kaypro1.dsk"
"$TD0_TOOL" "$ASSETS/kaypro1.td0" "$ASSETS/kaypro1.dsk"

dsk_size=$(wc -c < "$ASSETS/kaypro1.dsk" | tr -d ' ')
if [[ "$dsk_size" -ne "$KAYPRO_DSDD_SIZE" ]]; then
  log "unexpected .dsk size $dsk_size (expected $KAYPRO_DSDD_SIZE)"
  exit 1
fi

log "assets ready:"
ls -lh "$ASSETS/81-478a.rom" "$ASSETS/kaypro1.td0" "$ASSETS/kaypro1.dsk"
printf '\nRun emulator (from repo root; paths are relative to cwd):\n'
printf '  %s/kaypro/kaypro_run --rom %s/81-478a.rom --disk-a %s/kaypro1.dsk\n' \
  "$ROOT" "$ASSETS" "$ASSETS"
printf 'Bare filenames (81-478a.rom) only work if your cwd is %s\n' "$ASSETS"
