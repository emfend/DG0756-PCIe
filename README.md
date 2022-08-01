# Linux PCIe driver for Microchip PolarFire FPGA PCIe endpoint demo

This is the PCIe Linux driver for the "PolarFire FPGA PCIe Endpoint, DDR3 and DDR4 Memory Controller Data Plane Demo Guide" (DG0756).

This driver ships with the DG0756 design file archive, but targets an older kernel version.

In order for the driver to work with a current kernel version (5.19), some adjustments must be made.

Since Microchip's support team has declared that the driver has been released under GPL, the necessary changes are published in this repository.

> It's worth noting that the DG0756 driver only works on X86 platforms and an additional kernel quick is required to make this work on platforms where the kernel is responsible for BAR allocation (no firmware like BIOS).
The reason for this is that the design uses a PCI ID for which the kernel does not allocate the BARs.
This repository contains a patch that adds such a quirk to the Linux kernel (0001-PCI-add-class-quirk-for-Microchip-DG0756-to-fix-BAR-.patch).
