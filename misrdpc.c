/*******************************************************************************
 * (c) Copyright 2017-2018 Microsemi SoC Products Group. All rights reserved.
 *
 * File : misrdpc.c
 *
 * Description : Polarfire PCIe Endpoint for interrupt functionality
 *
********************************************************************************/

#include "mpcie.h"
#include "mhardware.h"

/**
* ms_pci_interrupt() - PCIe interrupt Handler
*
*  @irq : PCIe interrupt number
*/
irqreturn_t ms_pci_interrupt(int irq, void* dev_id)
{

	int lirq = 0, actualirq,test = 0;
	u32* ptr_local = NULL;
	struct pci_dev *dev = dev_id;
	struct privData *pciData;

	pciData = pci_get_drvdata(dev);

	lirq = pciData->pdev->irq;

	printk("ms_pci_interrupt --> %d\n",irq);

	if (pciData->Isr_Status == TRUE)
	{
		pciData->count++;
	}

	actualirq = irq - lirq;
	switch (actualirq)
	{
	case 0x0:
		pciData->isr1_count++;
		break;
	case 0x1:
		pciData->isr2_count++;
		break;
	case 0x2:
		pciData->isr3_count++;
		break;
	case 0x3:
		pciData->isr4_count++;
		break;
	}

	// clear IRQ
	ptr_local = (u32*)pciData->barInfo[0].baseVAddr;

	*(ptr_local + G5_DMA_ISTATUS_LOCAL_OFFSET) = 0XFFFFFFFF;

	*(ptr_local + G5_DMA_ISTATUS_HOST) = 0XFFFFFFFF;

	if(pciData->G5_CN_Dma_write_status == TRUE)
	{
		ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_STATUS;
		test = *ptr_local;
		if((test&0x1) == 0x1)
		{
			pciData->G5_CN_Dma_write_status = FALSE;

		}
		printk("irq G5_WDMA_STATUS %d\n", *ptr_local);
	}

	if(pciData->G5_CN_Dma_Read_Status == TRUE)
	{
		ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_STATUS;
		test = *ptr_local;
		if((test&0x1) == 0x1)
		{
			pciData->G5_CN_Dma_Read_Status = FALSE;
		}
		printk("irq G5_RDMA_STATUS %d\n", *ptr_local);
	}

	if(pciData->G5_SG_Dma_write_status == TRUE)
	{
		ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_STATUS;
		test = *ptr_local;
		if((test&0x1) == 0x1)
		{
			pciData->G5_SG_Dma_write_status = FALSE;

		}
		printk("irq G5_WDMA_STATUS %d\n", *ptr_local);
	}

	if(pciData->G5_SG_Dma_Read_Status == TRUE)
	{
		ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_STATUS;
		test = *ptr_local;
		if((test&0x1) == 0x1)
		{
			pciData->G5_SG_Dma_Read_Status = FALSE;

		}
		printk("irq G5_RDMA_STATUS %d\n", *ptr_local);
	}

	printk("irq cleared %d\n", irq);
	return IRQ_HANDLED;
}
