---
layout: post
title: Tomasulo Algorithm
tags: [architecture]
comments: true
---

Tomasulo algorithm is an out-of-order execution policy. The philosophy of
Tomasulo algorithm is to execute an operation as soon as the required conditions
are satisfied in spite of the order of instructions.

<!--more-->

A processor, for example, is executing the following three instructions:

```text
r2 = r0 / r1     [1]
r3 = r2 + r1     [2]
r6 = r4 + r5     [3]
```

In general, division is many times slower than addition. In the example, [1]
take a lot of cycles to complete the operation, and [2] has to wait for [1] to
get the value of r2, while [3] has no dependency on the result of
[1] or [2]. If the processor executes the instructions with out-of-order
execution policy, [3] may be finished by the adder before [2].

# Reservation station #

In Tomasulo algorithm, each functional unit (such as a multiplier or an adder)
has a reservation station that consists of many slots, each slots of which
corresponds to an instruction and contains the validity and value of the source
operands. The validity of a source operands indicates if its value is 
directly stored in the slot or the result of a previous instruction that has
not completed, in which situation the value is the index of the reservation
station slot that produces the result.

{% include svg.html name="reservation-station" caption="An example of reservation station" %}

In the example above, the two source operands in the first reservation station
slot are both ready, and hence the instruction can be executed as soon as the
functional unit is available. The second source operand in the second
reservation station slot is not available yet and its value is the result of the
computation on the first reservation station slot. Either source operand in the
third reservation station slot is not available yet and depends on the first and
second, respectively.

# Register file #

In addition to the values of the registers, the register file contains the
validity of each register (to indicate whether the value is directly stored in
the register or waiting to be computed by a reservation station slot) and the
index of the reservation station slot to produce the value (only valid when 
the value is waiting to be computed).

{% include svg.html name="register-file" caption="An example of register file" %}

In the example, the value of r0 and r1 is 0x3A and 0x3D, respectively. However,
the value of r2 has not been computed yet and its value is the result of the
operation on the second reservation station slot. 0x34 is the old value of the
register and hence meaningless in this situation.

# Implementation #

{% include svg.html name="implementation" caption="An example of simple implementation" %}

In Tomasulo algorithm, the life cycle of an instruction composes of three stages:
issue, execute and write back.

1. Issue: an instruction is assigned to a vacant reservation station slot if the
   reservation station is not full. For each source operand of the instruction,
   store the value directly in the reservation station if the value is valid in
   the register file (by checking the validity of the register). Otherwise,
   store the index of the reservation station slot that produces the value if
   the value is dependent on the result of a previous instruction.
   
2. Execute: execute the operation on a functional unit when all the source
   operands are valid and the function unit is not busy.
   
3. Write back: broadcast the result of the instruction on the common data bus.
   Write the value back to the destination register and update all the
   reservation station slots that wait for the value by setting the status to
   valid and the value to the result.

# How Tomasulo algorithm eliminate WAR and WAW hazard and minimizes RAW dependency.

In the situation that a write operation is followed by another write, RS of the
destination register is the index of the reservation station slot of the last
operation after all the instructions has issued, which ensures that the value of
the register is set to the result of the last write operation after all are
completed in spite of execution order.

If a write operation is followed by a read, when the write is issued, the value
of the register to be read by the latter operation is already put in the
reservation station. Therefore, it is safe to modify the register without
disturbing value of the read operation.

Under the circumstance that a read operation is followed by a write and, to be
more specific, the result of the latter is not available yet when the former is
issued, the RS and validity of the former is set to the index of the reservation
station slot of the latter and waiting. When the write operation is completed,
the result can be sent to the reservation station directly on the common data
bus, saving the cost that has to be spent on writing the register and reading back.
