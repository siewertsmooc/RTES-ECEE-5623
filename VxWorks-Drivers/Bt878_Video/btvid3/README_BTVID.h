/*
Dan Walkes
ECEN 5623
2-22-05

This document is a help file for the btvid3.c file.

btvid3.c is an attempt to clarify the code in btvid.c and add support for the Pentium 3 Asus targets using
the Intel 82801AA southbridge.

Note:  As of the 2/22 revision, this code will only work when the BT878 card is placed in a specific
motherboard slot.  In both the Pentium and Pentium III boards, the BT878 should be placed in slot 4.

-------
|     |
| CPU |
|     |
-------				      ___________
	      Slot1    Slot2   Slot3  | Slot4	|
		|	|	|     | |	|
		|	|	|     | |	|
		|	|	|     | |	|
		|	|	|     | |	|
				      |		|
				      ----------

This is due to limitations in the interrupt routing setup on the motherboard.  On the Pentium 3 systems,
the BIOS is setup to assign IRQ 3 to slot 4.

Note that this code supports only one capture card by default.  Other cards will require modification to the
code and most likely to the BIOS settings.

For future work, it would be interesting to attempt to use the interrupt routing tabe to dynamically configure the
card to work in any slot.  See findAndDecodePCIInterruptRoutingTable() in BTVIDDebug.c, also see:
http://www.microsoft.com/whdc/archive/pciirq.mspx

*/


