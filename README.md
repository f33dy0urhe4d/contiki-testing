contiki-testing --> TEMP_ETX
===============

--v1.1--
--------------

-->**hotfix:**
new metric defined which considers ETX as link path metric and takes into account the attribute "temperature" to favourite nodes that own the specified attribute in the parent selection.

-->**edited files:** (only the following files in the rpl folder have been changed)

/rpl/
- rpl.h
- rpl-conf.h

- rpl.c
- rpl-icmp6.c
- rpl-of-etx.c

-->**to-do list:**
-define the attribute *residual energy* to favourite nodes with the greatest attribute in the parent selection
-define a parameter that specifies how long a certain metric has to be active