esptool.py --chip esp32 merge_bin -o merged-firmware.bin \
    0x0 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x20000 build/ota-portal.bin \
    0x720000 build/www.bin
