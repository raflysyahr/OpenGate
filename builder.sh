#!/bin/bash

# autoBuild.sh
# Otomatis build ESP-IDF, lalu jika sukses: hapus file lama di SD card → copy file baru

TARGET_DIR="/sdcard/ESP32S3/ota-portal"
TARGET_OTA_AUTOMATION="/root/ota-automation/firmware"
FILES=(
    "build/bootloader/bootloader.bin"
    "build/partition_table/partition-table.bin"
    "build/ota-portal.bin"
    "build/www.bin"
)

echo "=================================="
echo "Mulai Auto Build ESP32-S3"
echo "Tanggal: $(date)"
echo "=================================="

# Langkah 1: Jalankan build
echo "Menjalankan: idf.py build"
idf.py build

# Cek apakah build sukses
if [ $? -eq 0 ]; then
    echo "Build SUKSES!"

    # Pastikan semua file hasil build memang ada
    ALL_FILES_EXIST=true
    for file in "${FILES[@]}"; do
        if [ ! -f "$file" ]; then
            echo "   ⚠ File tidak ditemukan: $file"
            ALL_FILES_EXIST=false
        fi
    done

    if [ "$ALL_FILES_EXIST" = false ]; then
        echo "Beberapa file binary tidak ada, copy dibatalkan."
        exit 1
    fi

    # Pastikan folder tujuan ada
    mkdir -p "$TARGET_DIR"

    # Hapus file-file lama di SD card terlebih dahulu
    echo "Menghapus file lama di $TARGET_DIR"
    for file in "${FILES[@]}"; do
        filename=$(basename "$file")
        target_file="$TARGET_DIR/$filename"
        if [ -f "$target_file" ]; then
            rm "$target_file"
            echo "   ✗ Dihapus: $target_file"
        fi
    done

    # Copy file-file baru ke SD card
    echo "Copy file hasil build terbaru ke $TARGET_DIR"
    for file in "${FILES[@]}"; do
        cp "$file" "$TARGET_DIR/"
	cp "$file" "$TARGET_OTA_AUTOMATION/"

        if [ $? -eq 0 ]; then
            echo "   ✓ $file → $TARGET_DIR"
	    echo "   ✓ $file → $TARGET_OTA_AUTOMATION"
        else
            echo "   ✗ Gagal copy $file"
	    echo "   ✗ Gagal copy $file"
        fi
    done

    echo "Selesai! Firmware terbaru sudah ada di SD card."
else
    echo "Build GAGAL!"
    echo "File di SD card TIDAK diubah (file lama tetap dipertahankan)."
    echo "Perbaiki error di kode terlebih dahulu."
fi

echo "=================================="
echo "Auto Build Selesai"
echo "=================================="
