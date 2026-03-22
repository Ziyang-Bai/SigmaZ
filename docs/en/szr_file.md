# SZR File Format

## Overview

`SZR` is SigmaZ's binary benchmark report format (default extension: `.szr`). It stores hardware information, raw scores, normalized scores, timeout flags, and the total score from a full benchmark run. You may upload `.szr` files to the [SigmaZ Leaderboard](https://sigmaz.retrolab.top) to share your scores with players worldwide.

The current format version (`0x00020001`) is serialized from a fixed-size packed struct with:

- Little-endian encoding
- Packed layout (`#pragma pack(push, 1)`)
- Full-buffer XOR obfuscation using a repeating 8-byte key
- A trailing 32-bit checksum calculated on plaintext data

## File Layout

### Fixed Size

- Total size: `828` bytes
- Python `struct` format:

```text
<4sI16s64s32s64s64sI64s64s128s128s128s6f6fIIII
```

### Field Definitions (V2.1)

| Offset | Type | Field | Description |
| :--- | :--- | :--- | :--- |
| 0 | `char[4]` | `magic` | Constant `SZR\x1A` |
| 4 | `uint32` | `version` | Format version, currently `0x00020001` |
| 8 | `char[16]` | `cpu_vendor` | CPU vendor string |
| 24 | `char[64]` | `cpu_name` | CPU brand/model |
| 88 | `char[32]` | `os_mode` | Runtime mode + process bitness (e.g. `[ Win32 / Modern Mode ] 32-bit`) |
| 120 | `char[64]` | `sys_version` | OS version string |
| 184 | `char[64]` | `cpu_features` | CPU feature summary |
| 248 | `uint32` | `cores` | Logical core count |
| 252 | `char[64]` | `cpu_signature` | CPUID signature summary |
| 316 | `char[64]` | `cpu_cache` | Cache information |
| 380 | `char[128]` | `motherboard` | Motherboard information |
| 508 | `char[128]` | `memory_info` | Memory information |
| 636 | `char[128]` | `bios_info` | BIOS information |
| 764 | `float[6]` | `raw_scores` | Raw benchmark scores |
| 788 | `float[6]` | `norm_scores` | Normalized scores (486=100) |
| 812 | `uint32` | `timeouts` | Timeout bitmask |
| 816 | `uint32` | `total_score` | Total score |
| 820 | `uint32` | `timestamp` | Unix timestamp (seconds) |
| 824 | `uint32` | `checksum` | Checksum |

## Score Array Order

Both `raw_scores` and `norm_scores` use the same index order:

1. Integer
2. Float
3. MemOps
4. Crypto
5. Compress
6. Matrix

## Timeout Bitmask (`timeouts`)

`timeouts` is a bitmask:

- `0x01`: Integer timed out
- `0x02`: Float timed out
- `0x04`: MemOps timed out
- `0x08`: Crypto timed out
- `0x10`: Compress timed out
- `0x20`: Matrix timed out

`0` means no timeout occurred.

## XOR Obfuscation

Before writing to disk, the entire struct buffer is XOR-obfuscated:

- Key: `"SigmaZ95"` (8 bytes)
- Rule: `data[i] ^= key[i % 8]`

Readers must apply the same XOR first to recover plaintext bytes before unpacking.

## Checksum Algorithm

Checksum is computed on the plaintext struct; the `checksum` field itself is excluded.

- Initial value: `0x7A69676D`
- Iteration: `sum = (sum << 1) + byte`
- Width: truncated to 32-bit unsigned

Pseudo code:

```text
sum = 0x7A69676D
for each byte in plain[0 : len-4]:
    sum = ((sum << 1) + byte) & 0xFFFFFFFF
```

## Compatibility Notes

- In V2.1, `os_mode` may include both platform mode and process bitness.
- This value is informational only and is not used in score calculation.

- Older `SZR` versions (for example `0x00010000`) use different layouts and cannot be parsed as V2.1 directly.
- Readers should validate `magic` and `version` first, then choose a version-specific parser.
- See `read_szr_report.py` in this repository for a V2.1 reference implementation.

## Reader Example

```bash
python read_szr_report.py report.szr
```

Force mode (ignore size/magic/checksum failures):

```bash
python read_szr_report.py report.szr --force
```
