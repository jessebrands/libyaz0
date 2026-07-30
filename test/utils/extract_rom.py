"""
    extract_rom.py: extracts yaz0 files from a Nintendo 64 Zelda ROM

    Copyright (C) 2026 Jesse Gerard Brands

    This file is part of libyaz0.

    libyaz0 is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    libyaz0 is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
    more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
"""

import os
import struct
import sys


def read_dma_table_entity(file):
    chunk = file.read(16)
    if len(chunk) < 16:
        return None

    return struct.unpack(">IIII", chunk)


def find_dma_table(file):
    file.seek(4192)

    while True:
        entity = read_dma_table_entity(file)
        if entity is None:
            return None
        if entity[0] == 0x0 and entity[1] == 0x1060:
            return file.tell() - 16


def read_dma_table(filename):
    if not os.path.exists(filename):
        return None

    with open(filename, 'rb') as file:
        dma_start = find_dma_table(file)

        if dma_start is None:
            return None
        else:
            print(f"Found DMA table at offset {dma_start}")

        # We found the first entry, which describes the MAKEROM.
        # What we want is the entry for the DMA table itself, located at the third entry.
        file.seek(dma_start + 32)
        dma_table_ent = read_dma_table_entity(file)
        if dma_table_ent is None:
            return None

        table_size = dma_table_ent[1] - dma_table_ent[0]
        table_count = table_size // 16

        print(f"DMA table contains {table_count} entries")

        # Seek back to the start of the table and begin reading the table
        file.seek(dma_start)
        table = []
        for i in range(table_count):
            entity = read_dma_table_entity(file)
            if entity is None:
                return None
            table.append(entity)

        return table


def is_yaz0_file(buf):
    return buf[0:4] == b"Yaz0"


def extract_compressed_file(entry, filename):
    with open(filename, 'rb') as file:
        start = entry[2]
        size = entry[3] - entry[2]

        if size <= 0:
            return None

        file.seek(start)
        data = file.read(size)
        if len(data) != size:
            print(f"WARNING: range {start:#x}-{start + size:#x} in {filename} is "
                  f"truncated ({len(data)} of {size} bytes); skipping",
                  file=sys.stderr)
            return None

        return data


def extract_uncompressed_file(start, size, filename):
    with open(filename, 'rb') as file:
        if size <= 0:
            return None

        file.seek(start)
        data = file.read(size)
        if len(data) != size:
            print(f"WARNING: range {start:#x}-{start + size:#x} in {filename} is "
                  f"truncated ({len(data)} of {size} bytes); skipping",
                  file=sys.stderr)
            return None

        return data


def main():
    if len(sys.argv) != 4:
        print("usage: python extract_rom.py compressed_rom uncompressed_rom destination",
              file=sys.stderr)
        sys.exit(1)

    filename_a = sys.argv[1]
    filename_b = sys.argv[2]
    out_path = sys.argv[3]

    if not os.path.isfile(filename_a):
        print(f"{filename_a} is not a file", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(filename_b):
        print(f"{filename_b} is not a file", file=sys.stderr)
        sys.exit(1)

    if os.path.exists(out_path) and not os.path.isdir(out_path):
        print(f"{out_path} is not a directory", file=sys.stderr)
        sys.exit(1)

    print("Seeking DMA table in compressed ROM")
    table_a = read_dma_table(filename_a)

    if not table_a:
        print("Could not locate DMA table in compressed ROM!",
              file=sys.stderr)
        sys.exit(1)

    print("Seeking DMA table in uncompressed ROM")
    table_b = read_dma_table(filename_b)

    if not table_b:
        print("Could not locate DMA table in uncompressed ROM",
              file=sys.stderr)
        sys.exit(1)

    if len(table_a) != len(table_b):
        print("Found DMA table in both ROMs, but tables are of different lengths!!",
              file=sys.stderr)
        sys.exit(1)

    if not os.path.exists(out_path):
        print(f"Creating output directory: {out_path}")
        os.makedirs(out_path)

    print(f"Extracting files to: {out_path}")

    file_count = 0
    for i in range(len(table_a)):
        file_a = extract_compressed_file(table_a[i], filename_a)
        if not file_a:
            continue

        if not is_yaz0_file(file_a):
            continue

        file_count += 1
        destination = os.path.join(out_path, f"{i:04X}.szs")
        with open(destination, "wb") as out_file:
            out_file.write(file_a)

        # Nintendo 64 Zelda games store files with padded bytes.
        # We need to ensure we actually get the real size, which is fortunately
        # stored in the Yaz0 header.
        header = struct.unpack('>4sIII', file_a[0:16])
        uncompressed_size = header[1]

        destination = os.path.join(out_path, f"{i:04X}.bin")
        file_b = extract_uncompressed_file(table_b[i][0], uncompressed_size, filename_b)
        if file_b and is_yaz0_file(file_b):
            continue

        if file_b:
            with open(destination, "wb") as out_file:
                out_file.write(file_b)

    print(f"Done! Extracted {file_count} files")


if __name__ == "__main__":
    main()
