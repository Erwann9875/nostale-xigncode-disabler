"""Synthetic header and CLI checks; no game or vendor files are needed or executed."""

import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

from inspect_file import InvalidPE, inspect_file, read_pe_headers


SCRIPT = Path(__file__).with_name("inspect_file.py")


def image_fixture(pe64=False, dll=False, machine=None):
    """Create header bytes and inert data, not a runnable test program."""
    data = bytearray(1024)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 128)
    data[128:132] = b"PE\0\0"
    optional_size = 240 if pe64 else 224
    if machine is None:
        machine = 0x8664 if pe64 else 0x014C
    struct.pack_into("<HHIIIHH", data, 132, machine, 1, 0, 0, 0, optional_size,
                     0x2002 if dll else 0x0002)
    struct.pack_into("<H", data, 152, 0x20B if pe64 else 0x10B)
    struct.pack_into("<I", data, 152 + 60, 512)
    struct.pack_into("<I", data, 152 + (108 if pe64 else 92), 16)
    section = 152 + optional_size
    data[section:section + 8] = b".data\0\0\0"
    struct.pack_into("<II", data, section + 16, 512, 512)
    data[512:] = b"\x5A" * 512
    return data


class HeaderTests(unittest.TestCase):
    def inspect(self, data):
        return read_pe_headers(io.BytesIO(data), len(data))

    def test_pe32_executable(self):
        self.assertEqual(self.inspect(image_fixture()), {
            "format": "PE32", "machine": "0x014C", "architecture": "x86",
            "section_count": 1, "is_dll": False,
        })

    def test_pe32_plus_dll(self):
        report = self.inspect(image_fixture(pe64=True, dll=True))
        self.assertEqual(report["format"], "PE32+")
        self.assertEqual(report["architecture"], "x64")
        self.assertTrue(report["is_dll"])

    def test_arm64_is_not_mislabelled_as_x64(self):
        self.assertEqual(self.inspect(image_fixture(pe64=True, machine=0xAA64))["architecture"], "ARM64")

    def test_unknown_machine_keeps_numeric_identifier(self):
        report = self.inspect(image_fixture(machine=0xF00D))
        self.assertEqual(report["machine"], "0xF00D")
        self.assertEqual(report["architecture"], "unrecognized")

    def test_truncated_inputs_fail_cleanly(self):
        for length in (0, 1, 63, 64, 128, 151, 153, 375, 415, 511, 1023):
            with self.subTest(length=length), self.assertRaises(InvalidPE):
                self.inspect(image_fixture()[:length])

    def test_signatures_are_checked(self):
        for offset in (0, 128):
            with self.subTest(offset=offset), self.assertRaises(InvalidPE):
                data = image_fixture()
                data[offset] = 0
                self.inspect(data)

    def test_pe_pointer_cannot_overlap_dos_or_point_outside_file(self):
        for offset in (0, 63, 1024, 0xFFFFFFFF):
            with self.subTest(offset=offset), self.assertRaises(InvalidPE):
                data = image_fixture()
                struct.pack_into("<I", data, 0x3C, offset)
                self.inspect(data)

    def test_section_count_is_bounded(self):
        for count in (0, 97, 65535):
            with self.subTest(count=count), self.assertRaises(InvalidPE):
                data = image_fixture()
                struct.pack_into("<H", data, 134, count)
                self.inspect(data)

    def test_optional_header_size_and_magic_are_checked(self):
        for size in (0, 1, 2, 95, 65535):
            with self.subTest(size=size), self.assertRaises(InvalidPE):
                data = image_fixture()
                struct.pack_into("<H", data, 148, size)
                self.inspect(data)
        data = image_fixture()
        struct.pack_into("<H", data, 152, 0x107)
        with self.assertRaisesRegex(InvalidPE, "Unsupported"):
            self.inspect(data)

    def test_directory_count_must_fit_optional_header(self):
        for pe64 in (False, True):
            with self.subTest(pe64=pe64), self.assertRaisesRegex(InvalidPE, "data-directory"):
                data = image_fixture(pe64=pe64)
                struct.pack_into("<I", data, 152 + (108 if pe64 else 92), 17)
                self.inspect(data)

    def test_headers_size_must_fit_file_and_cover_tables(self):
        for size in (0, 415, 1025):
            with self.subTest(size=size), self.assertRaisesRegex(InvalidPE, "SizeOfHeaders"):
                data = image_fixture()
                struct.pack_into("<I", data, 212, size)
                self.inspect(data)

    def test_raw_data_must_not_overlap_headers_or_exceed_file(self):
        for offset, size in ((0, 512), (511, 1), (512, 513), (0xFFFFFFFF, 512)):
            with self.subTest(offset=offset, size=size), self.assertRaisesRegex(InvalidPE, "raw data"):
                data = image_fixture()
                struct.pack_into("<II", data, 392, size, offset)
                self.inspect(data)

    def test_uninitialized_section_with_no_raw_data_is_allowed(self):
        data = image_fixture()
        struct.pack_into("<II", data, 392, 0, 0)
        self.assertEqual(self.inspect(data)["section_count"], 1)


class FileAndCliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / "test image \u6e2c\u8a66.dat"
        self.contents = bytes(image_fixture())
        self.path.write_bytes(self.contents)

    def run_cli(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), *map(str, args)],
                              capture_output=True, text=True, encoding="utf-8", timeout=10)

    def test_fingerprint_and_report_do_not_include_absolute_paths(self):
        report = inspect_file(self.path)
        self.assertEqual(report["schema_version"], 1)
        self.assertEqual(report["size_bytes"], len(self.contents))
        self.assertEqual(report["sha256"], hashlib.sha256(self.contents).hexdigest())
        self.assertEqual(report["file_name"], self.path.name)
        self.assertNotIn(str(self.root), json.dumps(report))

    def test_inspection_preserves_bytes_and_creates_no_report_file(self):
        inspect_file(self.path)
        self.assertEqual(self.path.read_bytes(), self.contents)
        self.assertEqual(list(self.root.iterdir()), [self.path])

    def test_cli_reports_unicode_filename_as_portable_json(self):
        result = self.run_cli(self.path)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(json.loads(result.stdout), inspect_file(self.path))

    def test_cli_rejects_missing_file_directory_and_invalid_image(self):
        invalid = self.root / "not-an-image.dat"
        invalid.write_bytes(b"not a PE image")
        for path in (self.root / "missing.dat", self.root, invalid):
            with self.subTest(path=path.name):
                result = self.run_cli(path)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, "")
                self.assertIn("Inspection failed:", result.stderr)
                self.assertNotIn("Traceback", result.stderr)

    def test_cli_help_needs_no_input_file(self):
        result = self.run_cli("--help")
        self.assertEqual(result.returncode, 0)
        self.assertIn("without loading or executing", result.stdout)


if __name__ == "__main__":
    unittest.main()
