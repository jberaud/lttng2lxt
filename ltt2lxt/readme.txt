Color code :

state                 |    gtkwave          |   gtkwave-parrot (-c)
PROCESS_IDLE          |    middle line (z)  |   middle line (z)
PROCESS_KERNEL        |    full (x)         |   1
PROCESS_USER          |    full (u)         |   full (x)
PROCESS_WAKEUP        |    full (w)         |   0
PROCESS_DEAD          |    0                |   0

SOFTIRQ_IDLE          |    middle line (z)  |   middle line (z)
SOFTIRQ_RUNNING       |    full (x)         |   full (x)
SOFTIRQ_RAISING       |    full (w)         |   0

IRQ_IDLE              |    middle line (z)  |   middle line (z)
IRQ_RUNNING           |    full (x)         |   full (x)
IRQ_PREEMPT           |    full (w)         |   0
