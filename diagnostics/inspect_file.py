"""Report a file's fingerprint and basic PE headers without loading or executing it."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys


class InvalidPE(ValueError):
    """The input does not satisfy the supported structural header checks."""


MACHINES = {
    0x014C: "x86",
    0x01C0: "ARM",
    0x01C4: "ARM Thumb-2",
    0x8664: "x64",
    0xAA64: "ARM64",
    0xA641: "ARM64EC",
    0xA64E: "ARM64X",
}


def read_at(stream, file_size, offset, count, label):
    if offset < 0 or count < 0 or offset + count > file_size:
        raise InvalidPE(f"Truncated or out-of-range {label}.")
    stream.seek(offset)
    data = stream.read(count)
    if len(data) != count:
        raise InvalidPE(f"Could not read the complete {label}.")
    return data


def read_pe_headers(stream, file_size):
    dos = read_at(stream, file_size, 0, 64, "DOS header")
    if dos[:2] != b"MZ":
        raise InvalidPE("Not a Windows PE image: the MZ signature is missing.")
    pe_offset = struct.unpack_from("<I", dos, 0x3C)[0]
    if pe_offset < 64:
        raise InvalidPE("The PE header overlaps the DOS header.")
    header = read_at(stream, file_size, pe_offset, 24, "PE signature/COFF header")
    if header[:4] != b"PE\0\0":
        raise InvalidPE("The PE signature is missing.")
    machine, section_count = struct.unpack_from("<HH", header, 4)
    optional_size, characteristics = struct.unpack_from("<HH", header, 20)
    if not 1 <= section_count <= 96:
        raise InvalidPE("Expected between 1 and 96 image sections.")

    optional = read_at(stream, file_size, pe_offset + 24, optional_size, "optional header")
    if len(optional) < 2:
        raise InvalidPE("The required image optional header is missing.")
    magic = struct.unpack_from("<H", optional)[0]
    formats = {0x10B: ("PE32", 96), 0x20B: ("PE32+", 112)}
    if magic not in formats:
        raise InvalidPE("Unsupported optional-header format; expected PE32 or PE32+.")
    format_name, minimum_size = formats[magic]
    if len(optional) < minimum_size:
        raise InvalidPE("The optional header is too short for its format.")
    directory_count = struct.unpack_from("<I", optional, minimum_size - 4)[0]
    if directory_count > (optional_size - minimum_size) // 8:
        raise InvalidPE("The data-directory table exceeds the optional header.")

    section_offset = pe_offset + 24 + optional_size
    section_bytes = read_at(stream, file_size, section_offset, section_count * 40, "section table")
    headers_size = struct.unpack_from("<I", optional, 60)[0]
    if not section_offset + len(section_bytes) <= headers_size <= file_size:
        raise InvalidPE("SizeOfHeaders does not contain the header tables within the file.")
    for index in range(section_count):
        raw_size, raw_offset = struct.unpack_from("<II", section_bytes, index * 40 + 16)
        if raw_size and (raw_offset < headers_size or raw_offset + raw_size > file_size):
            raise InvalidPE(f"Section {index + 1} has raw data outside the file's data area.")

    return {
        "format": format_name,
        "machine": f"0x{machine:04X}",
        "architecture": MACHINES.get(machine, "unrecognized"),
        "section_count": section_count,
        "is_dll": bool(characteristics & 0x2000),
    }


def inspect_file(path):
    path = Path(path)
    with path.open("rb") as stream:
        stream.seek(0, 2)
        file_size = stream.tell()
        pe = read_pe_headers(stream, file_size)
        stream.seek(0)
        digest = hashlib.sha256()
        bytes_read = 0
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
            bytes_read += len(block)
        if bytes_read != file_size:
            raise InvalidPE("The file size changed while being inspected; close writers and retry.")
    return {
        "schema_version": 1,
        "file_name": path.name,
        "size_bytes": file_size,
        "sha256": digest.hexdigest(),
        "pe": pe,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path, help="An existing PE image (.exe, .dll or executable .dat).")
    args = parser.parse_args(argv)
    try:
        report = inspect_file(args.file)
    except (OSError, InvalidPE) as error:
        print(f"Inspection failed: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
