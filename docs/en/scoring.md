# Scoring System

## System Baseline

SigmaZ's scoring system is benchmarked based on a typical high-end configuration from 1994, the **Intel 486 DX2-66** (estimated value). The standard score for this configuration is set to **100 points**.

## Calculation Formulas

Individual test scores are normalized:

```
Individual Score = ( Test Result / 486 Baseline Reference ) * 100
```

For example, if your system's integer processing speed is 50 times faster than a 486 DX2-66, the score for that item will be 5000 points.

## Reference Values Table

| Test Item | Unit | 486 Ref Value (100 pts) | Weight |
| :--- | :--- | :--- | :--- |
| INT (Integer) | OPS (Operations Per Second) | 84.0 | 20% |
| FLOAT (Float) | Iter/ms (Iteration Per Millisecond) | 170.0 | 20% |
| MEM OPS (Memory Operations) | MB/s (Megabytes Per Second) | 24.0 | 10% |
| CRYPTO (Crypto) | KB/s | 189.0 | 15% |
| COMPRESS (Compress) | KB/s | 0.58 | 15% |
| MATRIX (Matrix) | Matrices/s | 0.91 | 20% |

## Total Score Calculation

To prevent skewed total scores due to extremely high (or low) performance in a single category, the total score is calculated using a **weighted geometric mean**:

```
Total = exp( 
  0.20 * ln(Int) +
  0.20 * ln(Float) +
  0.10 * ln(Mem) +
  0.15 * ln(Crypto) +
  0.15 * ln(Compress) +
  0.20 * ln(Matrix)
)
```

## Timeout Scoring

For early or older hardware (such as 386/486), SigmaZ introduces a timeout scoring mechanism:

*   Integer, Float, Crypto, Compression, and Matrix tests stop automatically after **60 seconds**.
*   The Memory test uses a fixed duration window (**3 seconds**) to estimate stable bandwidth.
*   The program will calculate the score proportionally based on the amount of computation completed.