/*******************************************************************************
 * (c) Copyright 2017-2018 Microsemi SoC Products Group. All rights reserved.
 *
 * File : Mpcie.c
 *
 * Description : Linux Device Driver to access Polarfire PCIe Endpoint
 *
********************************************************************************/

#include "mpcie.h"
#include "mhardware.h"

struct privData *pciData;

struct cdev *ms_pci_cdev = NULL;
dev_t ms_pci_dev;
static struct class *ms_pci_class = NULL;
struct isr_count g_isr_count;

void interrupt_control(struct pci_dev *pdev, unsigned char);
struct pci_dev *samplePdev;
int i = 0,Devdetect = 0;


//Endpoint vendorID/DeviceID
static struct pci_device_id ms_pci_tbl[] = {
	{ MPCI_VENDOR_ID, MPCI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};


extern irqreturn_t ms_pci_interrupt(int irq, void* dev_id);
extern void *mem_vmalloc(unsigned long size);
extern int sgm_kernel_pages(struct sg_mapping_t *sg_map, char *start,u32 size);
extern struct sg_mapping_t* sg_list_create(u32 len);
extern void sg_destroy_mapper(struct sg_mapping_t *sg_map);
extern void *mem_vfree(void *mem,unsigned long size);

MODULE_DEVICE_TABLE(pci, ms_pci_tbl);


/**
* ms_pci_probe() - Initialize char device Module
*
*  @pdev : Platform Device
*  @ent  : PCI Device ID List Entry
*/

static int ms_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err = 0,nvec = 4;
	int i = 0;
	u32 *ptr_local = NULL;

   printk("Initializing %s \n", pci_name(pdev));

	/*Enable PCIe endpoint device */
 	err = pci_enable_device(pdev);
	if (err) {
		printk("pci_enable_device() failed, err \n");
		return err;
	}

	pciData = kzalloc(sizeof(struct privData), GFP_KERNEL);
	if (pciData == NULL) {
		printk("Unable to allocate memory for PCI resource\n");
		pci_disable_device(pdev);
		return err;
	}

	/*
	* Enable bus-mastering on device.
	*/
	pci_set_master(pdev);


	/*
	* Check for PCI resources
	*/

	err = pci_request_regions(pdev, "/dev/MS_PCI_DEV");

	if (err) {
		printk("Cannot obtain PCI resources :%d\n", err);
		kfree(pciData);
    pci_disable_device(pdev);
		return err;
	}

#ifdef MPCIE_DMA_ENABLE

		if(! (err = pci_set_dma_mask(pdev, DMA_BIT_MASK(64))))
		{
			/* System supports 64-bit DMA */
				printk("System supports 64-bit DMA\n");
		}
		else {
			if((err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32))))
			{
				printk("No usable DMA configuration \n");
				return 0;
			}
				printk("System supports 32-bit DMA\n");
		}

		pciData->vbuf = mem_vmalloc(1024*1024);

		if(pciData->vbuf == NULL)
		{
			printk("Unable to allocate kernal virtual buffer \n");

		}

		pciData->kbuf = (u32 *)kmalloc(1024*1024,GFP_DMA);

		if(pciData->kbuf == NULL)
		{
			printk("Unable to allocate kernal Continuous buffer \n");

		}

		pciData->dma_allocate_len = 1024*1024;

#endif

	pciData->isr1_count = 0;
	pciData->isr2_count = 0;
	pciData->isr3_count = 0;
	pciData->isr4_count = 0;


	pciData->barInfo[0].baseVAddr = pci_iomap(pdev, 0, 0);

	if(pciData->barInfo[0].baseVAddr == NULL) {
		printk(KERN_INFO "Unable to map BAR #0 address");
  }

	pciData->barInfo[2].baseVAddr  = pci_iomap(pdev, 2, 0);

	if(pciData->barInfo[2].baseVAddr == NULL) {
		printk(KERN_INFO "Unable to map BAR #2 address");
  }
//For Interrupts allocation
	nvec = pci_alloc_irq_vectors(pdev,1,nvec,PCI_IRQ_MSI);
	if(nvec < 0)
	{
		printk("unable to allocate irq 0x%x\n", pdev->irq);
	}
	printk("nvec = %d\n",nvec);
	printk("request irg:%d\n", pdev->irq);

	for(i = 0;i<nvec;i++)
	{
		if(request_irq(pdev->irq+i, ms_pci_interrupt, IRQF_SHARED, DRV_NAME, pdev)) {

			printk("unable to register irq 0x%x\n", pdev->irq);
		}
	}

	pciData->pdev=pdev;

	 /* Save private data pointer in device structure */
	pci_set_drvdata(pdev, pciData);

	ptr_local = (u32 *)pciData->barInfo[0].baseVAddr;

	if(pciData->barInfo[0].baseVAddr != NULL)
	{
		//For MSI
		*(ptr_local + G5_ECC_DISABLE_OFFSET) = G5_ECC_DISABLE_VAL;
		*(ptr_local + G5_SEC_ERROR_INT_OFFSET) = G5_SEC_ERROR_INT_VAL;
		*(ptr_local + G5_DED_ERROR_INT_OFFSET) = G5_DED_ERROR_INT_VAL;
		*(ptr_local + G5_PCIE_EVENT_INT_OFFSET) = G5_PCIE_EVENT_INT_VAL;
		*(ptr_local + G5_MSI_MASK_OFFSET) = G5_MSI_MASK_VAL;

		*(ptr_local + G5_DMA_ISTATUS_HOST) = G5_MSI_MASK_CLEAR_VAL;
		*(ptr_local + G5_DMA_ISTATUS_LOCAL_OFFSET) = G5_MSI_MASK_CLEAR_VAL;

	}

	Devdetect = TRUE;
	return PCI_SUCCESS;
}

/**
* ms_pci_remove() - Uninitialize the PCIe device
*
*  @pdev : Platform Device
*/
static void ms_pci_remove(struct pci_dev *pdev)
{
	printk("in ms_pci_remove");
 	pci_clear_master(pdev);
	pci_release_regions(pdev);
	pci_disable_device(pdev);


	if(pciData->vbuf != NULL)
	{
		mem_vfree(pciData->vbuf,pciData->dma_allocate_len);
	}

	if(pciData->kbuf != NULL)
	{
		kfree(pciData->kbuf);
	}


	for(i = 0;i<4;i++)
	{
		free_irq(pdev->irq+i, pdev);
	}

	pci_set_drvdata(pdev, NULL);
	kfree(pciData);
}

static struct pci_driver ms_pci_driver = {
		.name = DRV_NAME,
		.probe = ms_pci_probe,
		.remove = ms_pci_remove,
		.id_table = ms_pci_tbl,
};

/**
* char_dev_open() - Open Char Device
*
*  @inode : Device Driver Information
*  @file  : Pointer to the file struct
*/
static int char_dev_open(struct inode* inode,
	struct file* file)
{
	printk("open operation invoked \n");
	return SUCCESS;
}

/**
* char_dev_release() - Release Char Device
*
*  @inode : Device Driver Information
*  @file  : Pointer to the file struct
*/

static int char_dev_release(struct inode* inode,
	struct file* file)
{
	printk("close operation invoked \n");
	return SUCCESS;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int char_dev_ioctl(struct inode* i, struct file* f, u32 cmd, unsigned long arg)
#else
static long char_dev_ioctl(struct file* f, u32 cmd, unsigned long arg)
#endif
{
	int ret = 0, size = 0, bar = 0;
	struct config_bar local_cf;
	struct config_info local_info;
	u8 cap_ptr;
	u16 config_read = 0;
	struct device_info lmdev;
	struct isr_count local_isr;
	struct pci_config local_config;
	volatile u32* ptr_local = NULL;
	dma_addr_t bufPA;

	switch (cmd)
	{
	case MPCI_IOCBAR_WRITE:
		printk("in MPCI_IOCBAR_WRITE \n");
		if (copy_from_user(&local_cf, (struct config_bar *)arg, sizeof(struct config_bar)))
		{
			return -EACCES;
		}
		printk("in MPCI_IOCBAR_WRITE bar    = %u \n", local_cf.bar);
		printk("in MPCI_IOCBAR_WRITE offest = %u \n", local_cf.offset);
		printk("in MPCI_IOCBAR_WRITE val    = %u \n", local_cf.val);


		ptr_local = (u32*)pciData->barInfo[local_cf.bar].baseVAddr;
		*(ptr_local + local_cf.offset) = local_cf.val;

		break;
	case MPCI_IOCBAR_READ:
		printk("in MPCI_IOCBAR_READ \n");
		if (copy_from_user(&local_cf, (struct config_bar*)arg, sizeof(struct config_bar)))
		{
			return -EACCES;
		}
		printk("in MPCI_IOCBAR_WRITE bar    = %u \n", local_cf.bar);
		printk("in MPCI_IOCBAR_WRITE offest = %u \n", local_cf.offset);
		printk("in MPCI_IOCBAR_WRITE val    = %u \n", local_cf.val);

		ptr_local = (u32*)pciData->barInfo[local_cf.bar].baseVAddr;

		local_cf.val = *(ptr_local + local_cf.offset);

		ret = copy_to_user((struct config_bar*)arg, &local_cf, sizeof(struct config_bar));
		printk(KERN_INFO "ret = %d\n", ret);

		break;
	case MPCI_IOCCONFIG_WRITE:
		printk("in MPCI_IOCCONFIG_WRITE \n");
		break;
	case MPCI_IOCCONFIG_READ:
		printk("in MPCI_IOCCONFIG_READ \n");

		if (copy_from_user(&local_info, (struct config_info*)arg, sizeof(local_info)))
		{
			return -EACCES;
		}

		if (local_info.val_16)
		{

			pci_read_config_word(pciData->pdev, local_info.offset, &config_read);
			local_info.val_16 = config_read;
		}
		if (local_info.val_32)
		{

			pci_read_config_dword(pciData->pdev, local_info.offset, &local_info.val_32);
			//local_info.val_32 = config_read;
		}
		printk("PCI driver local_info.offset = %x\n", local_info.val_16);
		printk("PCI driver local_info.offset = %x\n", local_info.val_32);
		ret = copy_to_user((struct config_info*)arg, &local_info, sizeof(local_info));

		break;
	case MPCI_IOCCONFIG_READ_ALL:
		printk("in MPCI_IOCCONFIG_READ \n");

		if (copy_from_user(&local_config, (struct pci_config*)arg, sizeof(local_config)))
		{
			return -EACCES;
		}


		pci_read_config_word(pciData->pdev, PCI_VENDOR_ID, &local_config.VendorID);

		pci_read_config_word(pciData->pdev, PCI_DEVICE_ID, &local_config.DeviceID);

		pci_read_config_word(pciData->pdev, PCI_COMMAND, &local_config.Command);

		pci_read_config_word(pciData->pdev, PCI_STATUS, &local_config.Status);

		pci_read_config_dword(pciData->pdev, PCI_REVISION_ID, &local_config.RevisionID);

		pci_read_config_byte(pciData->pdev, PCI_CLASS_PROG, &local_config.Clase_prog);

		pci_read_config_word(pciData->pdev, PCI_CLASS_DEVICE, &local_config.Class_device);

		pci_read_config_byte(pciData->pdev, PCI_CACHE_LINE_SIZE, &local_config.CacheLineSize);

		pci_read_config_byte(pciData->pdev, PCI_LATENCY_TIMER, &local_config.LatencyTimer);

		pci_read_config_byte(pciData->pdev, PCI_HEADER_TYPE, &local_config.HeaderType);

		pci_read_config_byte(pciData->pdev, PCI_BIST, &local_config.BIST);

		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_0, &local_config.BaseAddresses[0]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[0]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_1, &local_config.BaseAddresses[1]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[1]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_2, &local_config.BaseAddresses[2]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[2]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_3, &local_config.BaseAddresses[3]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[3]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_4, &local_config.BaseAddresses[4]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[4]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_5, &local_config.BaseAddresses[5]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[5]);


		pci_read_config_dword(pciData->pdev, PCI_CARDBUS_CIS, &local_config.CIS);

		pci_read_config_word(pciData->pdev, PCI_SUBSYSTEM_VENDOR_ID, &local_config.Sub_VID);

		pci_read_config_word(pciData->pdev, PCI_SUBSYSTEM_ID, &local_config.Sub_DID);

		pci_read_config_dword(pciData->pdev, PCI_ROM_ADDRESS, &local_config.ROMBaseAddress);

		pci_read_config_byte(pciData->pdev, PCI_INTERRUPT_LINE, &local_config.IRQ_Line);

		pci_read_config_byte(pciData->pdev, PCI_INTERRUPT_PIN, &local_config.IRQ_Pin);

		pci_read_config_byte(pciData->pdev, PCI_MIN_GNT, &local_config.Min_Gnt);

		pci_read_config_byte(pciData->pdev, PCI_MAX_LAT, &local_config.Max_Lat);

		//For MSI
		cap_ptr = pci_find_capability(pciData->pdev,PCI_CAP_ID_MSI);

		pci_read_config_word(pciData->pdev, cap_ptr + PCI_MSI_FLAGS,(u16 *) &local_config.MessageControl);

		printk("MSIEnable = %x\n", local_config.MessageControl.MSIEnable);
		printk("MultipleMessageCapable = %x\n", local_config.MessageControl.MultipleMessageCapable);
		printk("PCI_MSI_FLAGS = %x\n", local_config.MessageControl.MultipleMessageEnable);

		printk("PCI driver Vendor ID = %x\n", local_config.VendorID);
		printk("PCI driver Device ID = %x\n", local_config.DeviceID);

		ret = copy_to_user ((struct pci_config *)arg,&local_config,sizeof(struct pci_config));

		break;
	case MPCI_IOCDEVICE_INFO:
		printk("in MPCI_IOCDEVICE_INFO \n" );

		if(Devdetect == TRUE)
		{
			lmdev.device_status = TRUE;
		}
		else
		{
			lmdev.device_status = FALSE;
		}

		lmdev.demo_type = 1;

		lmdev.device_type = POLARFIRE_EVAL;

		size = pci_resource_len(pciData->pdev, 0);

		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_0, &local_config.BaseAddresses[0]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[0]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_1, &local_config.BaseAddresses[1]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[1]);
		pci_read_config_dword(pciData->pdev, PCI_BASE_ADDRESS_2, &local_config.BaseAddresses[2]);
		printk("PCI driver local_info.BaseAddresses = %x\n", local_config.BaseAddresses[2]);
		//bar info
		lmdev.bar0_add =(long) local_config.BaseAddresses[0];
		lmdev.bar0_size =  size;
		lmdev.bar1_add = (long) local_config.BaseAddresses[1];
		lmdev.bar1_size = 0;
		size = pci_resource_len(pciData->pdev, 2);
		lmdev.bar2_add = (long)  local_config.BaseAddresses[2];
		lmdev.bar2_size = size;

		for(i=0; i<MAX_BARS; i++)
		 {
			/* Atleast BAR0 must be there. */
			if ((size = pci_resource_len(pciData->pdev, i)) == 0) {
			continue;
			}
			bar++;
		}

		lmdev.num_bar = bar;

		printk("demo_type = %d\n",lmdev.demo_type);
		printk("device_status = %d \n",lmdev.device_status);
		printk("device_type = %d \n",lmdev.device_type);
		printk("num_bar     = %d\n",lmdev.num_bar);
		printk("bar0_add     = 0x%lx\n",(long )pciData->barInfo[0].baseVAddr);
		printk("bar0_size     = %x\n",lmdev.bar0_size);
		printk("bar2_add     = 0x%lx\n",(long)pciData->barInfo[2].baseVAddr);
		printk("bar2_size     = %x\n",lmdev.bar2_size);

		ret = copy_to_user ((struct device_info *)arg,&lmdev,sizeof(struct device_info));

		break;
	case MPCI_IOCISR_INFO:
		printk("in MPCI_IOCISR_INFO \n" );

		if (copy_from_user( &local_isr,(struct local_isr *)arg, sizeof(local_isr)))
		{
			return -EACCES;
		}

		if( local_isr.isr_type_act == 1)
		{
			local_isr.isr1 = pciData->isr1_count;
			local_isr.isr2 = pciData->isr2_count;
			local_isr.isr3 = pciData->isr3_count;
			local_isr.isr4 = pciData->isr4_count;

		}
		else if(local_isr.isr_type_act == 2)
		{
			local_isr.isr1 = pciData->isr1_count = 0;
			local_isr.isr2 = pciData->isr2_count = 0;
			local_isr.isr3 = pciData->isr3_count = 0;
			local_isr.isr4 = pciData->isr4_count = 0;

		}
		else
		{
			return -EINVAL;
		}

		ret = copy_to_user ((struct local_isr*)arg,&local_isr,sizeof(local_isr));

		break;
	case MPCI_G5_BD_COUNT:
		{
			struct config_bar bd_ct = { 0,0,0 };

			bd_ct.offset = pciData->dma_bd_wcount;
			bd_ct.val = pciData->dma_bd_Rcount;

			ret = copy_to_user((struct config_bar*)arg, &bd_ct, sizeof(bd_ct));
		}
		break;
	case MPCI_G5_DMA_CONFIG:
		{

			G5_DmaRegisters dma_reg;
			printk("in MPCI_G5_DMA_CONFIG \n");
			if (copy_from_user(&dma_reg, (G5_DmaRegisters*)arg, sizeof(G5_DmaRegisters)))
			{
				return -EACCES;
			}

			pciData->g5dmaregisters.Dma_Srcaddr32 = dma_reg.Dma_Srcaddr32;
			pciData->g5dmaregisters.Dma_Destaddr32 = dma_reg.Dma_Destaddr32;
			pciData->g5dmaregisters.Dma_length = dma_reg.Dma_length;
		}
		break;
	case MPCI_G5_SG_DMA_WRITE:
		{
			DMA_ELEMENT dma_local = {0};
			PDMA_TRANSFER_ELEMENT dteVA ;
			u32 nents=0,offset=0,rc =0,i = 0,test = 0;
			struct sg_mapping_t *sg_map = NULL;
			struct scatterlist *sglist = NULL;
			dma_addr_t bus_addr,dteLA_sg;

			pciData->dma_bd_wcount = 0;

			printk(" dma_reg.Dma_Srcaddr32 = %d\n",pciData->g5dmaregisters.Dma_Srcaddr32);
			printk(" dma_reg.Dma_Destaddr32 = %d\n",pciData->g5dmaregisters.Dma_Destaddr32);
			printk(" dma_reg.Dma_length = %d\n",pciData->g5dmaregisters.Dma_length);

			if(pciData->vbuf == NULL)
			{
				printk("in MPCI_G5_SG_DMA_WRITE unable to allocate memory  \n");
				return -EACCES;
			}

			if(pciData->g5dmaregisters.Dma_length > (pciData->dma_allocate_len))
			{
				printk("in MPCI_G5_SG_DMA_WRITE allocated memory less then requested memory  \n");
				return -EACCES;
			}

			if (copy_from_user((char *)pciData->vbuf, (char *)arg, pciData->g5dmaregisters.Dma_length))
			{
				printk("in MPCI_G5_SG_DMA_WRITE unable to copy memory  \n");
				return -EACCES;
			}

					sg_map = sg_list_create(pciData->g5dmaregisters.Dma_length);

			if(sg_map == NULL)
			{
				printk("in MPCI_G5_SG_DMA_WRITE unable to create sg list  \n");
				return -EACCES;
			}

			rc = sgm_kernel_pages(sg_map, pciData->vbuf, pciData->g5dmaregisters.Dma_length);

			if(rc < 0)
			{
				printk("in MPCI_G5_SG_DMA_WRITE unable to get kernel_pages  \n");
				return -EACCES;
			}

			sglist = sg_map->sgl;

			nents = pci_map_sg(pciData->pdev, sg_map->sgl, sg_map->mapped_pages, PCI_DMA_TODEVICE);

			printk("nents = %d\n",nents);

	    dteVA = (DMA_TRANSFER_ELEMENT *) pci_alloc_consistent(pciData->pdev,(nents * sizeof(DMA_TRANSFER_ELEMENT)),&bus_addr);

			bufPA =	pci_map_single(pciData->pdev,dteVA,(nents * sizeof(DMA_TRANSFER_ELEMENT)),PCI_DMA_TODEVICE);
			dteLA_sg = bufPA;

			for (i=0; i < nents; i++) {

	        // Construct this DTE.

				dteLA_sg += sizeof(DMA_TRANSFER_ELEMENT);

				dteVA->Desc_page_size     = sg_dma_len(&sglist[i]);
				dteVA->Desc_type		  		= 0x0;
				dteVA->Desc_pre_status    = 0x0;
				dteVA->Desc_Next_Rdy      = 0x1;
				dteVA->Desc_next_addr32   = (unsigned long)dteLA_sg >> 5;
				dteVA->Desc_next_addr64   = 0;
				dteVA->Desc_src_addr32    = (unsigned long)sg_dma_address(&sglist[i]);
				dteVA->Desc_src_addr64    = 0;
				dteVA->Desc_dest_addr32   = pciData->g5dmaregisters.Dma_Destaddr32+offset;
				dteVA->Desc_dest_addr64   = 0;
				dteVA->Desc_Se_Cond = 0;
				dteVA->Desc_irq = 0x0;

				offset += sg_dma_len(&sglist[i]);

				pciData->dma_bd_wcount +=1;

					if (i == (nents - 1)) {

					dteVA->Desc_Se_Cond = 1;
					dteVA->Desc_next_addr32   = 0;
					dteVA->Desc_next_addr64	  = 0;
					printk("--> MPCI_G5_SG_DMA_WRITE last element \n");
				break;
				}
				dteVA++;
			}

#if 1
				//set write discriptor address

				pciData->G5_SG_Dma_write_status = TRUE;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_OFFSET;

				*(ptr_local) =  0x0000ffff;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_HOST_OFFSET;

				*(ptr_local) = 0xff0000ff;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr)+ G5_PCIE_EVENT_INT_OFFSET;
				*(ptr_local) = G5_PCIE_EVENT_INT_VAL;

				dma_local.Dma_Srcparam = 0x00000000;
				dma_local.Dma_Destparam = 0x00000004;
				dma_local.Dma_Srcaddr32 = bufPA;
				dma_local.Dma_Srcaddr64 = 0;
				dma_local.Dma_Destaddr32 = 0x00000000;// not required for DMA 0
				dma_local.Dma_Destaddr64 = 0x00000000;
				dma_local.Dma_length = pciData->g5dmaregisters.Dma_length;
				dma_local.Dma_Control = 0x01003329;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCPARAM;
				*(ptr_local) =  dma_local.Dma_Srcparam;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTPARAM;
				*(ptr_local) =  dma_local.Dma_Destparam;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCADDR32;
				*(ptr_local) =  dma_local.Dma_Srcaddr32;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCADDR64;
				*(ptr_local) =  dma_local.Dma_Srcaddr64;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTADDR32;
				*(ptr_local) =  dma_local.Dma_Destaddr32;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTADDR64;
				*(ptr_local) =  dma_local.Dma_Destaddr64;

				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_LENGTH;
				*(ptr_local) =  dma_local.Dma_length;

				ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x38/4;
				*(ptr_local) =  dma_local.Dma_length;


				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_CONTROL;
				*(ptr_local) =  dma_local.Dma_Control;

				// counter start for throughput
				ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x18/4;
				*(ptr_local) =  0xA1;
#endif
				mdelay(10);
#if 1
				while(1)
				{
					ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_STATUS;
					test = *ptr_local;
					if(pciData->G5_SG_Dma_write_status == FALSE )
					{
							pci_unmap_single(pciData->pdev,bufPA,pciData->g5dmaregisters.Dma_length,PCI_DMA_TODEVICE);
							printk("in ioctl G5_WDMA_STATUS %d\n", test);
							sg_destroy_mapper(sg_map);
							pciData->G5_SG_Dma_write_status = FALSE;
						break;

					}
				}
#endif
			}
		break;
	case MPCI_G5_SG_DMA_READ:
	{

		DMA_ELEMENT dma_local = {0};
		PDMA_TRANSFER_ELEMENT dteVA ;
		u32 nents=0,offset=0,length=0,rc =0,i = 0,test = 0,test_local = 1;
		struct sg_mapping_t *sg_map ;
		struct scatterlist *sglist ;
		dma_addr_t bus_addr,dteLA_sg;

		printk("in MPCI_G5_SG_DMA_READ \n");

		pciData->dma_bd_Rcount = 0;

		printk(" dma_reg.Dma_Srcaddr32 = %d\n",pciData->g5dmaregisters.Dma_Srcaddr32);
		printk(" dma_reg.Dma_Destaddr32 = %d\n",pciData->g5dmaregisters.Dma_Destaddr32);
		printk(" dma_reg.Dma_length = %d\n",pciData->g5dmaregisters.Dma_length);

		if(pciData->vbuf == NULL)
		{
			printk("in MPCI_G5_SG_DMA_READ unable to allocate memory  \n");
			return -EACCES;
		}

		if(pciData->g5dmaregisters.Dma_length > (pciData->dma_allocate_len))
		{
			printk("in MPCI_G5_SG_DMA_READ allocated memory less then requested memory  \n");
			return -EACCES;
		}

		sg_map = sg_list_create(pciData->g5dmaregisters.Dma_length);
		rc = sgm_kernel_pages(sg_map, pciData->vbuf, pciData->g5dmaregisters.Dma_length);
		sglist = sg_map->sgl;

		nents = pci_map_sg(pciData->pdev, sg_map->sgl, sg_map->mapped_pages, PCI_DMA_FROMDEVICE);
		dteVA = (DMA_TRANSFER_ELEMENT *) pci_alloc_consistent(pciData->pdev,(nents * sizeof(DMA_TRANSFER_ELEMENT)),&bus_addr);

		bufPA =	pci_map_single(pciData->pdev,dteVA,(nents * sizeof(DMA_TRANSFER_ELEMENT)),PCI_DMA_FROMDEVICE);
		dteLA_sg = bufPA;

		for (i=0; i < nents; i++)  {

				// Construct this DTE.
		dteLA_sg += sizeof(DMA_TRANSFER_ELEMENT);


		dteVA->Desc_page_size     = sg_dma_len(&sglist[i]);
		dteVA->Desc_type		  		= 0x0;
		dteVA->Desc_pre_status    = 0x0;
		dteVA->Desc_Next_Rdy      = 0x1;
		dteVA->Desc_next_addr32   = (unsigned int )dteLA_sg >>5;
		dteVA->Desc_next_addr64   = 0;
		dteVA->Desc_src_addr32    = pciData->g5dmaregisters.Dma_Srcaddr32+offset;
		dteVA->Desc_src_addr64    = 0;
		dteVA->Desc_dest_addr32   =(unsigned long)sg_dma_address(&sglist[i]);
		dteVA->Desc_dest_addr64   = 0;
		dteVA->Desc_Se_Cond       = 0;
		dteVA->Desc_irq = 0x0;

		offset += sg_dma_len(&sglist[i]);
		length += sg_dma_len(&sglist[i]);
		pciData->dma_bd_Rcount +=1;

		if (i == (nents - 1)){


		dteVA->Desc_Se_Cond       = 1;
		dteVA->Desc_next_addr32   = 0;
		dteVA->Desc_next_addr64	  = 0;

			if(test_local == 1)
			{
					// for lsram access
					ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + 0x8648/4;
					*(ptr_local) = 0x30000000;
					mdelay(10);
					//msleep(10);
					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x000/4;
					*(ptr_local) = 0;

					ptr_local = (u32*)  (pciData->barInfo[2].baseVAddr) + 0x004/4;
					*(ptr_local) = length<<8;

					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x008/4;
					*(ptr_local) = 0x1;

					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x00c/4;
					*(ptr_local) = 0x0;

					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x010/4;
					*(ptr_local) = pciData->g5dmaregisters.Dma_Srcaddr32;

					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x014/4;
					*(ptr_local) =0x00;

					ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x018/4;
					*(ptr_local) = dteVA->Desc_dest_addr32;

					ptr_local = (u32*)  (pciData->barInfo[2].baseVAddr) + 0x01c/4;
					*(ptr_local) = dteVA->Desc_dest_addr64;
					test_local = 0;
					mdelay(10);
					//msleep(10);
					// for default  access
					ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + 0x8648/4;
				*(ptr_local) = 0x10000000;
				}
				break;
			}

			dteVA++;
		}

#if 1
			pciData->G5_SG_Dma_Read_Status = TRUE;
			ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_OFFSET;

			*(ptr_local) =0xffff;

			ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_HOST_OFFSET;

			*(ptr_local) =0xff0000ff;

			dma_local.Dma_Srcparam = 0x00000004;
			dma_local.Dma_Destparam = 0x00000000;
			dma_local.Dma_Srcaddr32 = 0x30000000;// LSRAM address
			dma_local.Dma_Srcaddr64 = 0x00000000;
			dma_local.Dma_Destaddr32 = bufPA;
			dma_local.Dma_Destaddr64 = 0;
			dma_local.Dma_length = pciData->g5dmaregisters.Dma_length;
			dma_local.Dma_Control = 0x10003329;

				// write into device registers for scatter gather disc

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCPARAM;
			*(ptr_local) = dma_local.Dma_Srcparam;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTPARAM;
			*(ptr_local) = dma_local.Dma_Destparam;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCADDR32;
			*(ptr_local) = dma_local.Dma_Srcaddr32;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCADDR64;
			*(ptr_local) = dma_local.Dma_Srcaddr64;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTADDR32;
			*(ptr_local) = dma_local.Dma_Destaddr32;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTADDR64;
			*(ptr_local) = dma_local.Dma_Destaddr64;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_LENGTH;
			*(ptr_local) = dma_local.Dma_length;

				ptr_local = (u32*) (pciData->barInfo[0].baseVAddr) + G5_RDMA_CONTROL;
			*(ptr_local) = dma_local.Dma_Control;
				// counter start for throughput
				ptr_local = (u32*) (pciData->barInfo[2].baseVAddr) + 0x18/4;
			*(ptr_local) = 0xA2;

#endif
		mdelay(10);
#if 1
				while(1)
				{
					ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_STATUS;
					test = *ptr_local;
					if(((test&0x1) == 0x1) || pciData->G5_SG_Dma_Read_Status == FALSE )
					{
						pciData->G5_SG_Dma_Read_Status = FALSE;
						pci_unmap_single(pciData->pdev,bufPA,pciData->g5dmaregisters.Dma_length,PCI_DMA_FROMDEVICE);
						if (copy_to_user((u32*)arg,(u32*)pciData->vbuf, pciData->g5dmaregisters.Dma_length))
						{
							return -EACCES;
						}
						//mem_vfree(buf,pciData->g5dmaregisters.Dma_length);
						sg_destroy_mapper(sg_map);
						//mem_vfree(buf,pciData->g5dmaregisters.Dma_length);
			break;
					}
				}
#endif
		}
		break;
	case MPCI_G5_CN_DMA_WRITE:
		{
			DMA_ELEMENT dma_local = {0};
			u32 test = 0;
			printk("in MPCI_G5_CN_DMA_WRITE \n");

			printk(" dma_reg.Dma_Srcaddr32 = %d\n",pciData->g5dmaregisters.Dma_Srcaddr32);
			printk(" dma_reg.Dma_Destaddr32 = %d\n",pciData->g5dmaregisters.Dma_Destaddr32);
			printk(" dma_reg.Dma_length = %d\n",pciData->g5dmaregisters.Dma_length);


			if(pciData->kbuf == NULL)
			{
				printk("in MPCI_G5_CN_DMA_WRITE unable to allocate memory  \n");
				return -EACCES;
			}

			if(pciData->g5dmaregisters.Dma_length > (pciData->dma_allocate_len))
			{
				printk("in MPCI_G5_CN_DMA_WRITE allocated memory less then requested memory  \n");
				return -EACCES;
			}


			if (copy_from_user((u32 *)pciData->kbuf, (u32 *)arg, pciData->g5dmaregisters.Dma_length))
			{
				return -EACCES;
			}

			bufPA = pci_map_single(pciData->pdev,pciData->kbuf,pciData->g5dmaregisters.Dma_length,PCI_DMA_TODEVICE);

			if (pci_dma_mapping_error(pciData->pdev,bufPA)) {
			/*
			* reduce current DMA mapping usage,
			* delay and try again later or
			* reset driver.
			*/
				//kfree(buf);
				printk("in ioctl MPCI_G5_CN_DMA_WRITE pci_dma_mapping_error \n");
				return -EACCES;

			}

			ptr_local = (u32*)pciData->barInfo[0].baseVAddr;

			pciData->G5_CN_Dma_write_status = TRUE;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_OFFSET;

			*(ptr_local) = 0xffff;//sathish0xffffffff

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_HOST_OFFSET;

			*(ptr_local) = 0xff0000ff;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_PCIE_EVENT_INT_OFFSET;
			*(ptr_local) = G5_PCIE_EVENT_INT_VAL;


			dma_local.Dma_Srcparam = 0x00000000;
			dma_local.Dma_Destparam = 0x00000004;
			dma_local.Dma_Srcaddr32 = bufPA;
			dma_local.Dma_Srcaddr64 = 0;
			dma_local.Dma_Destaddr32 = pciData->g5dmaregisters.Dma_Destaddr32;
			dma_local.Dma_Destaddr64 = 0x00000000;
			dma_local.Dma_length = pciData->g5dmaregisters.Dma_length;
			dma_local.Dma_Control = 0x00003321;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCPARAM;
			*(ptr_local) = dma_local.Dma_Srcparam;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTPARAM;
			*(ptr_local) = dma_local.Dma_Destparam;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCADDR32;
			*(ptr_local) = dma_local.Dma_Srcaddr32;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_SRCADDR64;
			*(ptr_local) = dma_local.Dma_Srcaddr64;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTADDR32;
			*(ptr_local) = dma_local.Dma_Destaddr32;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_DESTADDR64;
			*(ptr_local) = dma_local.Dma_Destaddr64;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_LENGTH;
			*(ptr_local) = dma_local.Dma_length;

			ptr_local = (u32*)(pciData->barInfo[2].baseVAddr) + 0x38/4;
			*(ptr_local) = dma_local.Dma_length;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_CONTROL;
			*(ptr_local) = dma_local.Dma_Control;

			// counter for dma throughput
			ptr_local = (u32*)(pciData->barInfo[2].baseVAddr) + 0x18/4;
			*(ptr_local) = 0xA1;
			mdelay(10);
#if 1
			while(1)
			{
				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_WDMA_STATUS;
				test = *ptr_local;
				if( pciData->G5_CN_Dma_write_status == FALSE )
				{
						pci_unmap_single(pciData->pdev,bufPA,pciData->g5dmaregisters.Dma_length,PCI_DMA_TODEVICE);
						//kfree(buf);
						printk("in ioctl G5_WDMA_STATUS %d\n", test);
						break;

				}
			}
#endif
		}
		break;
	case MPCI_G5_CN_DMA_READ:
		{
			DMA_ELEMENT dma_local = { 0 };
			u32 test  = 0;

			printk("in MPCI_G5_CN_DMA_READ \n");

			printk(" dma_reg.Dma_Srcaddr32 = %d\n",pciData->g5dmaregisters.Dma_Srcaddr32);
			printk(" dma_reg.Dma_Destaddr32 = %d\n",pciData->g5dmaregisters.Dma_Destaddr32);
			printk(" dma_reg.Dma_length = %d\n",pciData->g5dmaregisters.Dma_length);

			if(pciData->kbuf == NULL)
			{
				printk("in MPCI_G5_CN_DMA_READ unable to allocate memory  \n");
				return -EACCES;
			}

			if(pciData->g5dmaregisters.Dma_length > (pciData->dma_allocate_len))
			{
				printk("in MPCI_G5_CN_DMA_READ allocated memory less then requested memory  \n");
				return -EACCES;
			}

			pciData->G5_CN_Dma_Read_Status = TRUE;

			bufPA = pci_map_single(pciData->pdev,pciData->kbuf,pciData->g5dmaregisters.Dma_length,PCI_DMA_FROMDEVICE);

			// MASK intt
			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_OFFSET;
			*(ptr_local) = 0xffff;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_DMA_IMASK_HOST_OFFSET;
			*(ptr_local) = 0xff0000ff;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_PCIE_EVENT_INT_OFFSET;
			*(ptr_local) = G5_PCIE_EVENT_INT_VAL;

			dma_local.Dma_Srcparam = 0x00000004;
			dma_local.Dma_Destparam = 0x00000000;
			dma_local.Dma_Srcaddr32 = pciData->g5dmaregisters.Dma_Srcaddr32;
			dma_local.Dma_Srcaddr64 = 0x00000000;
			dma_local.Dma_Destaddr32 = bufPA;
			dma_local.Dma_Destaddr64 = 0;
			dma_local.Dma_length =  pciData->g5dmaregisters.Dma_length;;
			dma_local.Dma_Control = 0x00003321;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCPARAM;
			*(ptr_local) = dma_local.Dma_Srcparam;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTPARAM;
			*(ptr_local) = dma_local.Dma_Destparam;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCADDR32;
			*(ptr_local) = dma_local.Dma_Srcaddr32;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_SRCADDR64;
			*(ptr_local) = dma_local.Dma_Srcaddr64;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTADDR32;
			*(ptr_local) = dma_local.Dma_Destaddr32;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_DESTADDR64;
			*(ptr_local) = dma_local.Dma_Destaddr64;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_LENGTH;
			*(ptr_local) = dma_local.Dma_length;

			ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_CONTROL;
			*(ptr_local) = dma_local.Dma_Control;


			ptr_local = (u32*)(pciData->barInfo[2].baseVAddr) + 0x18/4;
			*(ptr_local) = 0xA2;

			mdelay(10);
			//wait for dma to complete
#if 1
			while(1)
			{
				ptr_local = (u32*)(pciData->barInfo[0].baseVAddr) + G5_RDMA_STATUS;
				test = *ptr_local;
				if(pciData->G5_CN_Dma_Read_Status == FALSE )
				{
					pci_unmap_single(pciData->pdev,bufPA,pciData->g5dmaregisters.Dma_length,PCI_DMA_FROMDEVICE);

					if (copy_to_user((u32 *)arg,(u32 *)pciData->kbuf, pciData->g5dmaregisters.Dma_length))
					{

						return -EACCES;
					}
					//kfree(buf);
					break;
				}
			}
#endif
		}
		break;
		default:
		printk("in default \n" );
		return -EINVAL;
    }

    return 0;
}


static struct file_operations char_dev_fops = {
	.owner = THIS_MODULE,
	.open = char_dev_open,
	.release = char_dev_release,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
   	.ioctl = char_dev_ioctl,
#else
    .unlocked_ioctl = char_dev_ioctl,
#endif
};

/**
*  pcidrv_init() - Initialize PCIe Module
*
*  Registers Platform Driver
*/

static __init int pcidrv_init(void)
{
	int ret,count=1;

	printk("Module Inserted \n");

	if(pci_register_driver(&ms_pci_driver)){
		printk("unable to register pci driver \n");
	return -1;
	}
	/* Request dynamic allocation of a device major number */
	if(alloc_chrdev_region (&ms_pci_dev, 0, count, PCI_DEV_NAME) < 0) {
    printk (KERN_ERR "failed to reserve major/minor range\n");
    return -1;
		}

	printk("MS_PCI_DEV:  Got Major %d\n", MAJOR(ms_pci_dev));

	if (!(ms_pci_cdev = cdev_alloc ())) {
    printk (KERN_ERR "cdev_alloc() failed\n");
    unregister_chrdev_region (ms_pci_dev, count);
    return -1;
	}
	/* Connect the file operations with cdev*/
	cdev_init(ms_pci_cdev,&char_dev_fops);

	/* Connect the majot/mionr number to the cdev */
	ret=cdev_add(ms_pci_cdev,ms_pci_dev,count);
	if( ret < 0 ) {
		printk("Error registering device driver\n");
		cdev_del(ms_pci_cdev);
		unregister_chrdev_region(ms_pci_dev, count);
		return -1;
	}
	/* Populate sysfs entry */
	ms_pci_class = class_create(THIS_MODULE, "VIRTUAL");

	/* Send uevents to udev, So it will create /dev nodes */
	device_create(ms_pci_class, NULL, ms_pci_dev, "%s", PCI_DEV_NAME);

	printk("Successfully created /dev/MS_PCI_DEV\n");

	printk("\nDevice Registered: %s\n", PCI_DEV_NAME);
	printk(KERN_INFO "Major number = %d, Minor number = %d\n", MAJOR(ms_pci_dev), MINOR(ms_pci_dev));

	return 0;
}

/**
* pcidrv_cleanup() - Uninitialize the PCIe Module
*
*  Unregister the Platform Driver
*/

static __exit void  pcidrv_cleanup(void)
{
	printk("Module Deleted \n");
	/* Unregister PCI driver */
	pci_unregister_driver(&ms_pci_driver);
	/* Remove the cdev */
	cdev_del(ms_pci_cdev);
	/* Release the major number */
	unregister_chrdev_region(ms_pci_dev, 1);
	/* Destroy device and class */
	device_destroy(ms_pci_class, ms_pci_dev);
	class_destroy(ms_pci_class);

	printk("\n Driver unregistered \n");
}

module_init(pcidrv_init);
module_exit(pcidrv_cleanup);

MODULE_AUTHOR("Durga Prasad");
MODULE_DESCRIPTION("PolarFire PCI Endpoint Device Driver");
MODULE_LICENSE("GPL");
