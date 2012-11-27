contiki-testing --> TIMING_OPTIONS
===============

--v1.1-- (TBD)
--------------

-->**hotfix:**
new param which defines how long a certain metric has to be considered ACTIVE, by triggering a timer by the udp-sink side.

-->**edited files:** (only the following files in the rpl folder have been changed)

/rpl/
- rpl.h
- rpl-conf.h

/examples/ipv6/rpl-collect/
- udp-sink.c

-->**to-do list:** (TBD)

-->**issues:** (TBD)