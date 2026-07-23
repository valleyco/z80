#!/usr/bin/env bash
# Download Kaypro TD0s, ROMs, and selected toolkit ZIPs from retroarchive.org,
# convert TeleDisk images to native SSDD/DSDD raw .dsk, and rewrite SSDD → DSDD
# via cpmtools for use under Universal CP/M.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS="$ROOT/kaypro/assets"
ROM_DIR="$ASSETS/rom"
TD0_DIR="$ASSETS/td0"
ZIP_DIR="$ASSETS/zip"
IMG_SSDD="$ASSETS/images/ssdd"
IMG_DSDD="$ASSETS/images/dsdd"
MANIFEST="$ASSETS/MANIFEST.txt"

TD0_TOOL="$ROOT/tools/td0_to_dsk/td0_to_dsk"
SSDD_TO_DSDD="$ROOT/tools/ssdd_to_dsdd"

BASE_DISKS="http://www.retroarchive.org/maslin/disks/kaypro"
BASE_ROMS="http://www.retroarchive.org/maslin/roms/kaypro"

SSDD_SIZE=204800
DSDD_SIZE=409600

# SS→DS convert exclusions (non-CP/M or known bad for kpii→kpiv).
EXCLUDE_SS_CONVERT=(copwrdos)

TD0_NAMES=(
  k10fload k10frlod k10grld1 k10grld2 k10grld3 k10grld4 k10grld5 k10grld6
  k10grld7 k10grld8 k10grld9 k10grlda k10grldb k10grldc k10grldd k10grlde
  k10hload k10u-rld k10urlod k2-22g k2x22g k4836765 k4836768 kaypro1
  kii-6085 kii-ssdd kpivdsdd kp-trom kp22gdsd kpii-149 kpii-old kpro-ii
  kprossdd kpws-330 pro884mx copwrcpm copwrdos k2g-blnk kii-blnk kii-mbas
  kii-plan kii-star kii-tw kii-ws33 kpmex114
)

ROM_NAMES=(
  81-146a.rom 81-149b.rom 81-149c.rom 81-187.rom 81-188e.rom 81-232.rom
  81-235.rom 81-277.rom 81-292a.rom 81-302c.rom 81-326.rom 81-356.rom
  81-478a.rom 81-x015.rom 884max.rom pro8-3.rom pro8-5.rom pro884mx.rom
  robie-u.rom trom34.rom pro884v5.rom
)

ZIP_NAMES=(urmtkit1 kp4diag k10udiag)

log() { printf '==> %s\n' "$*"; }
warn() { printf 'warning: %s\n' "$*" >&2; }

in_exclude() {
  local name="$1" x
  for x in "${EXCLUDE_SS_CONVERT[@]}"; do
    [[ "$name" == "$x" ]] && return 0
  done
  return 1
}

download() {
  local url="$1"
  local dest="$2"
  if [[ -f "$dest" ]]; then
    log "already have $(basename "$dest"), skipping download"
    return 0
  fi
  log "downloading $(basename "$dest")"
  if ! curl -fsSL --retry 3 --retry-delay 2 -o "$dest.partial" "$url"; then
    rm -f "$dest.partial"
    warn "failed to download $url (skipping)"
    return 1
  fi
  mv "$dest.partial" "$dest"
}

mkdir -p "$ROM_DIR" "$TD0_DIR" "$ZIP_DIR" "$IMG_SSDD" "$IMG_DSDD"

log "Kaypro assets -> $ASSETS"

# --- ROMs ---
rom_ok=0
rom_skip=0
for rom in "${ROM_NAMES[@]}"; do
  if download "$BASE_ROMS/$rom" "$ROM_DIR/$rom"; then
    rom_ok=$((rom_ok + 1))
  else
    rom_skip=$((rom_skip + 1))
  fi
done

# --- ZIPs ---
for zip in "${ZIP_NAMES[@]}"; do
  if download "$BASE_DISKS/${zip}.zip" "$ZIP_DIR/${zip}.zip"; then
    log "unzipping ${zip}.zip"
    mkdir -p "$ZIP_DIR/$zip"
    unzip -o -q "$ZIP_DIR/${zip}.zip" -d "$ZIP_DIR/$zip"
  fi
done

# --- Build converter ---
if [[ ! -x "$TD0_TOOL" ]]; then
  log "building td0_to_dsk"
  make -C "$ROOT/tools/td0_to_dsk" >/dev/null
fi
chmod +x "$SSDD_TO_DSDD" 2>/dev/null || true

# --- TD0 → images ---
: > "$MANIFEST"
{
  echo "# Kaypro assets manifest"
  echo "# generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "# layout: rom/ td0/ zip/ images/{ssdd,dsdd}/"
  echo
} >> "$MANIFEST"

td0_ok=0
td0_fail=0
ssdd_n=0
dsdd_n=0
convert_n=0
convert_skip=0

for name in "${TD0_NAMES[@]}"; do
  td0="$TD0_DIR/${name}.td0"
  if ! download "$BASE_DISKS/${name}.td0" "$td0"; then
    td0_fail=$((td0_fail + 1))
    echo "TD0 $name: DOWNLOAD_FAILED" >> "$MANIFEST"
    continue
  fi

  tmp_dsk=$(mktemp "$ASSETS/.tmp_${name}_XXXXXX.dsk")
  if ! "$TD0_TOOL" --format auto "$td0" "$tmp_dsk" 2>"$ASSETS/.tmp_${name}.log"; then
    warn "td0_to_dsk failed for $name"
    cat "$ASSETS/.tmp_${name}.log" >&2 || true
    rm -f "$tmp_dsk" "$ASSETS/.tmp_${name}.log"
    td0_fail=$((td0_fail + 1))
    echo "TD0 $name: CONVERT_FAILED" >> "$MANIFEST"
    continue
  fi
  # Show converter summary on stderr of the tool (already in log)
  cat "$ASSETS/.tmp_${name}.log" >&2 || true
  rm -f "$ASSETS/.tmp_${name}.log"

  size=$(wc -c < "$tmp_dsk" | tr -d ' ')
  status="ok"
  native=""

  if [[ "$size" -eq "$SSDD_SIZE" ]]; then
    native="ssdd"
    mv -f "$tmp_dsk" "$IMG_SSDD/${name}.dsk"
    ssdd_n=$((ssdd_n + 1))
    if in_exclude "$name"; then
      warn "skipping SS→DS convert for $name (excluded)"
      convert_skip=$((convert_skip + 1))
      status="ssdd (no dsdd convert; excluded)"
    elif [[ -x "$SSDD_TO_DSDD" ]] || [[ -f "$SSDD_TO_DSDD" ]]; then
      if "$SSDD_TO_DSDD" "$IMG_SSDD/${name}.dsk" "$IMG_DSDD/${name}.dsk"; then
        convert_n=$((convert_n + 1))
        status="ssdd + dsdd (cpmtools kpii→kpiv)"
      else
        warn "ssdd_to_dsdd failed for $name (keeping ssdd only)"
        status="ssdd (dsdd convert failed)"
        convert_skip=$((convert_skip + 1))
      fi
    else
      warn "ssdd_to_dsdd missing; keeping ssdd only for $name"
      status="ssdd (no converter)"
    fi
  elif [[ "$size" -eq "$DSDD_SIZE" ]]; then
    native="dsdd"
    mv -f "$tmp_dsk" "$IMG_DSDD/${name}.dsk"
    dsdd_n=$((dsdd_n + 1))
    status="dsdd (native)"
  else
    warn "unexpected size $size for $name (expected $SSDD_SIZE or $DSDD_SIZE)"
    rm -f "$tmp_dsk"
    td0_fail=$((td0_fail + 1))
    echo "TD0 $name: BAD_SIZE=$size" >> "$MANIFEST"
    continue
  fi

  td0_ok=$((td0_ok + 1))
  echo "TD0 $name: $status size=$size native=$native" >> "$MANIFEST"
done

{
  echo
  echo "# summary"
  echo "roms_ok=$rom_ok roms_skipped=$rom_skip"
  echo "td0_ok=$td0_ok td0_failed=$td0_fail"
  echo "images_ssdd=$ssdd_n images_dsdd_native=$dsdd_n"
  echo "ss_to_ds_converted=$convert_n ss_to_ds_skipped=$convert_skip"
} >> "$MANIFEST"

log "assets ready (see $MANIFEST)"
log "summary: roms=$rom_ok td0=$td0_ok ssdd=$ssdd_n dsdd_native=$dsdd_n converted=$convert_n"

printf '\nRun emulator (from repo root):\n'
printf '  %s/kaypro/kaypro_run \\\n' "$ROOT"
printf '    --rom %s/rom/81-478a.rom \\\n' "$ASSETS"
printf '    --disk-a %s/images/dsdd/kaypro1.dsk\n' "$ASSETS"
