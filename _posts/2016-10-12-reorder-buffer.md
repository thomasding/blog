---
layout: post
title: Reorder Buffer
tags: [architecture]
---

In the plain Tomasulo algorithm introduced by [Tomasulo Algorithm][1], the
changes applied to the registers and memory may take place in any order. That
may cause inconsistent state in the case of an exception, in which the execution
can not be resumed after the exception is handled.
Using reorder buffer the changes are restricted to be applied in-order in spite
of the actual execution order inside the out-of-order execution unit.

<!--more-->

Consider that a process is executing the following instructions in Tomasulo
algorithm:

```text
r2 = r0 / r1           [1]
r3 = r0 + r2           [2]
r4 = r4 + r5           [3]
```

Given that division is many times slower than addition, and instruction [3] has
no dependency on either [1] or [2], [3] is likely to complete before [1] and
[2]. Such execution order introduces a scenario that r4 has been updated to the
result of [3] while r2 and r3 remain the old values. Should an exception raise
at that time, it would result in a state that is neither after the completion of
instruction [1] nor [3].

Apart from exception handling, it also brings in problems when branch prediction
unit is functioning. When meeting a branching instruction, such as a conditional
jump, the processor may take the most probable branch to follow. Unluckily, if
the branch taken is proved to be wrong after several cycles, some registers may
already be overwritten. In order that branch prediction unit works properly, the
processor must provide a rollback approach to cancel all the executed
instructions after a branching point.

## In-order commit ##

Besides the three stages of Tomasulo algorithm, namely, **issue**, **execution**
and **write back**, a new stage called **commit** is introduced at the end of
the pipeline. All the intermediate results are written to the reorder buffer
at the **write back** stage, and then committed in the order of the
instruction sequence. Should any exception happen or branch prediction be proven
wrong, the buffered uncommitted instructions would be invalidated at once.

## Implementation ##

A reorder buffer is usually implemented as a ring buffer. Each item in the
reorder buffer records the type, destination register or memory address, value
and other information of an instruction. Furthermore, the reservation station
stores the index of the reorder buffer of the source operands instead of the
index of the other reservation station.

Here is a simplified implementation of Tomasulo algorithm with reorder buffer
(reorder buffer is abbreviated as ROB, and destination as Dst):

{% include svg.html name="implementation" caption="An example of reorder buffer" %}

At the moment shown in the figure, although the instruction of ROB3 has
completed, the result cannot be written to the register file because ROB1 and
ROB2 have not been committed, which ensures that the effects of instructions are
taken in order.

The four-stage implementation of Tomasulo algorithm with reorder buffer can be
described as:

1. **Issue**: put an instruction into the pipeline if the reorder buffer and reservation
   station are available. If the source operand is valid or the ROB of the
   operand is waiting for commit, store the value in the reservation station.
   Otherwise, store the index of the ROB in the reservation station.
2. **Execution**: if all the source operands of an instruction in the reservation
   station are ready and the functional unit is available, send the instruction
   to the functional unit.
3. **Write back**: after the computation is completed, broadcast the result on the
   common data bus and update all the reservation stations that depend on the
   value as well as the destination ROB.
4. **Commit**: if the next ROB is done, commit the result to the register file.

[1]: {% post_url 2016-10-10-tomasulo-algorithm %}
