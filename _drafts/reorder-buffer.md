---
layout: post
title: Reorder Buffer
category: Instruction Architecture
---

# Precise exception handling #

In the plain Tomasulo algorithm introduced by [?], the changes applied to the
registers and memory may take place in any order. That may cause unexpected
inconsistent state in case of an exception.

Consider that a process is executing the following instructions in Tomasulo
algorithm:

```text
r2 = r0 / r1           [1]
r3 = r0 + r2           [2]
r4 = r4 + r5           [3]
```

Given that division is many times slower than addition and instruction [3] has
no dependency on either [1] or [2], [3] is likely to complete before [1] and
[2]. Such execution order introduces a scenario that r6 has been updated to the
result of [3] while r2 and r3 remain the old values. Should an exception raise
then, it would be hard to decide from which instruction to resume the execution
after the exception is handled.

