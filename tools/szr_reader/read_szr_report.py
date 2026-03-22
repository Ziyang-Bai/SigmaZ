#!/usr/bin/env python3
import argparse
import datetime as dt
import struct
import sys

KEY = b"SigmaZ95"
MAGIC = b"SZR\x1A"
VERSION = 0x00020001
SALT = 0x7A69676D
FMT = "<4sI16s64s32s64s64sI64s64s128s128s128s6f6fIIII"
SIZE = struct.calcsize(FMT)


def xor_data(data: bytes) -> bytes:
    out = bytearray(data)
    for i in range(len(out)):
        out[i] ^= KEY[i % len(KEY)]
    return bytes(out)


def calc_checksum(plain: bytes) -> int:
    total = SALT
    for b in plain[:-4]:
        total = ((total << 1) + b) & 0xFFFFFFFF
    return total


def cstr(raw: bytes) -> str:
    return raw.split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()


def format_time(ts: int) -> str:
    if ts == 0:
        return "N/A"
    try:
        return dt.datetime.fromtimestamp(ts).strftime("%Y-%m-%d %H:%M:%S")
    except (OverflowError, OSError, ValueError):
        return f"Invalid({ts})"


def read_report(path: str, force: bool = False) -> dict:
    with open(path, "rb") as f:
        data = f.read()

    warnings = []

    if len(data) != SIZE:
        if not force:
            raise ValueError(f"文件大小错误: 期望 {SIZE} 字节, 实际 {len(data)} 字节")
        if len(data) < SIZE:
            warnings.append(f"强制模式：文件过短，已补零到 {SIZE} 字节")
            data = data + (b"\x00" * (SIZE - len(data)))
        else:
            warnings.append(f"强制模式：文件过长，仅解析前 {SIZE} 字节")
            data = data[:SIZE]

    plain = xor_data(data)
    fields = struct.unpack(FMT, plain)

    magic = fields[0]
    version = fields[1]
    cpu_vendor = cstr(fields[2])
    cpu_name = cstr(fields[3])
    os_mode = cstr(fields[4])
    sys_version = cstr(fields[5])
    cpu_features = cstr(fields[6])
    cores = fields[7]
    cpu_signature = cstr(fields[8])
    cpu_cache = cstr(fields[9])
    motherboard = cstr(fields[10])
    memory_info = cstr(fields[11])
    bios_info = cstr(fields[12])
    raw_scores = fields[13:19]
    norm_scores = fields[19:25]
    timeouts = fields[25]
    total_score = fields[26]
    timestamp = fields[27]
    checksum = fields[28]

    valid_magic = magic == MAGIC
    if not valid_magic and not force:
        raise ValueError(f"Magic 不匹配: {magic!r}")
    if not valid_magic and force:
        warnings.append(f"强制模式：Magic 不匹配 ({magic!r})")

    expected_checksum = calc_checksum(plain)
    valid_checksum = expected_checksum == checksum
    if not valid_checksum and not force:
        raise ValueError("Checksum 校验失败：报告可能已损坏或被篡改")
    if not valid_checksum and force:
        warnings.append(
            f"强制模式：Checksum 不匹配 (file=0x{checksum:08X}, calc=0x{expected_checksum:08X})"
        )

    return {
        "version": version,
        "cpu_vendor": cpu_vendor,
        "cpu_name": cpu_name,
        "os_mode": os_mode,
        "sys_version": sys_version,
        "cpu_features": cpu_features,
        "cores": cores,
        "cpu_signature": cpu_signature,
        "cpu_cache": cpu_cache,
        "motherboard": motherboard,
        "memory_info": memory_info,
        "bios_info": bios_info,
        "raw_scores": raw_scores,
        "norm_scores": norm_scores,
        "timeouts": timeouts,
        "total_score": total_score,
        "timestamp": timestamp,
        "checksum": checksum,
        "valid_magic": valid_magic,
        "valid_checksum": valid_checksum,
        "warnings": warnings,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Read and verify SigmaZ .SZR report")
    parser.add_argument("file", help="Path to .szr file")
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="Force parse arbitrary file (ignore size/magic/checksum failures)",
    )
    args = parser.parse_args()

    try:
        info = read_report(args.file, force=args.force)
    except Exception as exc:
        print(f"[ERROR] {exc}")
        return 1

    labels = ["Integer", "Float", "MemOps", "Crypto", "Compress", "Matrix"]

    if info["valid_magic"] and info["valid_checksum"]:
        print("SigmaZ Report OK")
    else:
        print("SigmaZ Report Parsed (FORCED)")
    
    print(f"Version      : 0x{info['version']:08X}")
    if info["version"] != VERSION:
        print(f"Warning      : 读取到非预期版本 (expected 0x{VERSION:08X})")
    
    print(f"CPU Vendor   : {info['cpu_vendor']}")
    print(f"CPU Name     : {info['cpu_name']}")
    print(f"CPU Signature: {info['cpu_signature']}")
    print(f"CPU Features : {info['cpu_features']}")
    print(f"CPU Cache    : {info['cpu_cache']}")
    print(f"Cores        : {info['cores']}")
    print(f"Mode/Bitness : {info['os_mode']}")
    print(f"OS Version   : {info['sys_version']}")
    print(f"Motherboard  : {info['motherboard']}")
    print(f"Memory Info  : {info['memory_info']}")
    print(f"BIOS Info    : {info['bios_info']}")
    print(f"Timestamp    : {info['timestamp']} ({format_time(info['timestamp'])})")
    print(f"Total Score  : {info['total_score']}")
    
    if info["timeouts"] > 0:
        print(f"Timeouts     : 0x{info['timeouts']:08X} (Some tests failed or timed out)")

    print("\nTest Results:")
    print(f"{'Test':<10} | {'Raw Score':<12} | {'Norm Score':<12}")
    print("-" * 38)
    for i, label in enumerate(labels):
        raw = info["raw_scores"][i]
        norm = info["norm_scores"][i]
        print(f"{label:<10} | {raw:<12.4f} | {norm:<12.4f}")

    if info["warnings"]:
        print("\nWarnings:")
        for item in info["warnings"]:
            print(f"  - {item}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
