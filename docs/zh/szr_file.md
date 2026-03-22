# SZR 文件说明

## 概述

`SZR` 是 SigmaZ 的二进制测试报告格式，默认扩展名为 `.szr`。该格式用于保存一次完整测试后的硬件信息、原始分数、归一化分数、超时标志与总分。你可以将 `.szr` 文件上传到[SigmaZ 天梯榜](https://sigmaz.retrolab.top)与全球玩家分享你的分数。

当前版本（`0x00020001`）由固定长度结构体序列化得到，采用：

- 小端字节序（Little-endian）
- 紧凑对齐（`#pragma pack(push, 1)`）
- 全文件按 8 字节密钥循环 XOR 混淆
- 末尾 32 位校验和（配合 XOR 前的明文计算）

## 文件布局

### 固定长度

- 总长度：`828` 字节
- 结构格式（Python `struct`）：

```text
<4sI16s64s32s64s64sI64s64s128s128s128s6f6fIIII
```

### 字段定义

| 偏移 | 类型 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | `char[4]` | `magic` | 固定为 `SZR\x1A` |
| 4 | `uint32` | `version` | 格式版本，当前 `0x00020001` |
| 8 | `char[16]` | `cpu_vendor` | CPU 厂商字符串 |
| 24 | `char[64]` | `cpu_name` | CPU 品牌型号 |
| 88 | `char[32]` | `os_mode` | 运行模式 + 进程位数（例如 `[ Win32 / Modern Mode ] 32-bit`） |
| 120 | `char[64]` | `sys_version` | 系统版本字符串 |
| 184 | `char[64]` | `cpu_features` | CPU 特性摘要 |
| 248 | `uint32` | `cores` | 逻辑核心数 |
| 252 | `char[64]` | `cpu_signature` | CPUID 签名摘要 |
| 316 | `char[64]` | `cpu_cache` | 缓存信息 |
| 380 | `char[128]` | `motherboard` | 主板信息 |
| 508 | `char[128]` | `memory_info` | 内存信息 |
| 636 | `char[128]` | `bios_info` | BIOS 信息 |
| 764 | `float[6]` | `raw_scores` | 原始测试分数 |
| 788 | `float[6]` | `norm_scores` | 归一化分数（486=100） |
| 812 | `uint32` | `timeouts` | 超时位掩码 |
| 816 | `uint32` | `total_score` | 总分 |
| 820 | `uint32` | `timestamp` | Unix 时间戳（秒） |
| 824 | `uint32` | `checksum` | 校验和 |

## 分数字段顺序

`raw_scores` 与 `norm_scores` 的下标顺序一致：

1. Integer
2. Float
3. MemOps
4. Crypto
5. Compress
6. Matrix

## 超时位掩码（`timeouts`）

`timeouts` 使用按位标志：

- `0x01`: Integer 超时
- `0x02`: Float 超时
- `0x04`: MemOps 超时
- `0x08`: Crypto 超时
- `0x10`: Compress 超时
- `0x20`: Matrix 超时

`0` 表示没有项目超时。

## XOR 混淆

写入磁盘前，整份结构体会被循环异或：

- 密钥：`"SigmaZ95"`（8 字节）
- 规则：`data[i] ^= key[i % 8]`

读取时需先执行同样 XOR 还原明文，再进行结构解析。

## 校验和算法

校验基于“明文结构体”计算，`checksum` 字段本身不参与累计。

- 初值：`0x7A69676D`
- 迭代：`sum = (sum << 1) + byte`
- 宽度：按 32 位无符号整数截断

伪代码：

```text
sum = 0x7A69676D
for each byte in plain[0 : len-4]:
    sum = ((sum << 1) + byte) & 0xFFFFFFFF
```

## 兼容性说明

- `os_mode` 在 V2.1 中可同时包含“平台模式 + 进程位数”信息。
- 该信息仅用于展示/记录，不参与评分计算。

- 旧版本 `SZR`（例如 `0x00010000`）字段布局不同，不能按 V2.1 直接解析。
- 建议读取器先检查 `magic` 与 `version`，再按对应版本分支解析。
- 仓库内提供脚本 `read_szr_report.py` 作为 V2.1 的参考实现。

## 读取示例

```bash
python read_szr_report.py report.szr
```

强制模式（忽略长度/Magic/校验失败）:

```bash
python read_szr_report.py report.szr --force
```
