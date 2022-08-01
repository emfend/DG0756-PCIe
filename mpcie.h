/*******************************************************************************
 * (c) Copyright 2017-2018 Microsemi SoC Products Group. All rights reserved.
 *
 * File : mpcie.h
 *
 * Description : Specific to kernel and application interface
 *
********************************************************************************/
#include <linux/kernel.h>			/* Need for KERN_INFO */
#include <linux/version.h>
#include <linux/module.h>			/* Needed by all modules */
#include <linux/fs.h>
#include <linux/init.h>			/* Need for the macros */
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <linux/pci_regs.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>


#define DRV_NAME "MS_PCI_DRIVER"
#define PCI_DEV_NAME "MS_PCI_DEV"

#define MPCI_VENDOR_ID  0x11aa
#define MPCI_DEVICE_ID  0x1556

#define PCI_BAR_0 0
#define PCI_BAR_2 2

#define MAX_BARS 6

#define PCI_SUCCESS 0L
#define PCI_FAILURE 1L

#define TRUE 1
#define FALSE 0

#define SUCCESS 0L

#define MPCIE_DMA_ENABLE 1

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

enum {
	POLARFIRE_EVAL = 1,
	POLARFIRE_SPL,
};

/*
 * Ioctl definitions
 */

 /* Use 'k' as magic number */
#define MPCI_IOC_MAGIC  'm'
/* Please use a different 8-bit number in your code */

#define MPCI_IOCBAR_WRITE    		_IOWR(MPCI_IOC_MAGIC, 0, int)
#define MPCI_IOCBAR_READ 			_IOWR(MPCI_IOC_MAGIC, 1, int)
#define MPCI_IOCCONFIG_WRITE		_IOWR(MPCI_IOC_MAGIC, 2, int)
#define MPCI_IOCCONFIG_READ 		_IOWR(MPCI_IOC_MAGIC, 3, int)
#define MPCI_IOCCONFIG_READ_ALL		_IOWR(MPCI_IOC_MAGIC, 4, int)
#define MPCI_IOCDEVICE_INFO 		_IOWR(MPCI_IOC_MAGIC, 5, int)
#define MPCI_IOCISR_INFO 			_IOWR(MPCI_IOC_MAGIC, 6, int)

#define MPCI_G5_DMA_CONFIG			_IOWR(MPCI_IOC_MAGIC, 11, int)
#define MPCI_G5_SG_DMA_WRITE		_IOWR(MPCI_IOC_MAGIC, 12, int)
#define MPCI_G5_SG_DMA_READ			_IOWR(MPCI_IOC_MAGIC, 13, int)
#define MPCI_G5_CN_DMA_WRITE		_IOWR(MPCI_IOC_MAGIC, 14, int)
#define MPCI_G5_CN_DMA_READ			_IOWR(MPCI_IOC_MAGIC, 15, int)
#define MPCI_G5_BD_COUNT			_IOWR(MPCI_IOC_MAGIC, 16, int)
#define MPCI_G5_DMA_TRANSFER_LENGTH	_IOWR(MPCI_IOC_MAGIC, 17, int)



/** Structure used in IOCTL to set the normal dma registers */
typedef struct {
	u16 DmaSize;
	u16 DmaDirStart;
	u16 DmaCh0Status;
	u16 DmaCh1Status;
	u16 DmaClkCount;
	u16 DmaAddWrite;
	u16 DmaAddRead;
}DmaRegisters;

typedef struct {
	u32       Dma_Srcaddr32;
	u32       Dma_Srcaddr64;
	u32       Dma_Destaddr32;
	u32       Dma_Destaddr64;
	u32       Dma_length;
	u32       Dma_Status;

}G5_DmaRegisters;

typedef struct {
	u32 G5_SG_Dma_write_status;
	u32 G5_SG_Dma_Read_Status;
	u32 G5_CN_Dma_write_status;
	u32 G5_CN_Dma_Read_Status;
	u32 G5_SG_Dma_Write_Length;
	u32 G5_SG_Dma_Read_Length;
	u32 G5_CN_FDMA_Status;
	u32 G5_SG_DDR_Roffset;
	u32 G5_SG_DDR_Woffset;
} G5_Dma_local_status;


struct config_bar {

	u32 bar;
	u32 offset;
	u32 val;

};

struct isr_count
{
	u32 isr1;
	u32 isr2;
	u32 isr3;
	u32 isr4;
	u32 isr_type_act;
};

struct config_info {
	int offset;
	u16 val_16;
	u32 val_32;
};


struct device_info
{
	int device_status;
	int device_type;
	int demo_type;
	int support_width;
	int n_width;
	int support_speed;
	int n_speed;
	int num_bar;
	u32 bar0_add;
	int bar0_size;
	u32 bar1_add;
	int bar1_size;
	u32 bar2_add;
	int bar2_size;
	u32 bar3_add;
	int bar3_size;
	u32 bar4_add;
	int bar4_size;
	u32 bar5_add;
	int bar5_size;
	u32 bar6_add;
	int bar6_size;
};

struct pci_config {

	u16  VendorID;
	u16  DeviceID;
	u16  Command;
	u16  Status;
	u32  RevisionID;
	u8   Clase_prog;
	u16  Class_device;
	u8   CacheLineSize;
	u8   LatencyTimer;
	u8   HeaderType;
	u8   BIST;
	u32  BaseAddresses[6];
	u32  CIS;
	u16  Sub_VID;
	u16  Sub_DID;
	u32  ROMBaseAddress;
	u8   IRQ_Line;
	u8   IRQ_Pin;
	u8   Min_Gnt;
	u8   Max_Lat;

	struct _PCI_MSI_MESSAGE_CONTROL {
		u16  MSIEnable : 1;
		u16  MultipleMessageCapable : 3;
		u16  MultipleMessageEnable : 3;
		u16  CapableOf64Bits : 1;
		u16  PerVectorMaskCapable : 1;
		u16  Reserved : 7;
	} MessageControl;

};

typedef struct {

	u32       Dma_Srcparam;
	u32       Dma_Destparam;
	u32       Dma_Srcaddr32;
	u32       Dma_Srcaddr64;
	u32       Dma_Destaddr32;
	u32       Dma_Destaddr64;
	u32       Dma_length;
	u32       Dma_Control;
	u32       Dma_Status;
	u32       Dma_Prc_Length;
	u32       Dma_Share_Access;
} DMA_ELEMENT, *PDMA_ELEMENT;

#pragma pack(1)
typedef struct {

	u32       Desc_status_num :4;
	u32       Desc_pre_status :4;
	u32       Desc_pre_page_size :24;
	u32       Desc_status_req :1;
	u32       Desc_type :3;
	u32       Desc_irq :4;
	u32       Desc_page_size :24;
	u32	   		Desc_Se_Cond :4;
	u32       Desc_Next_Rdy :1;
	u32       Desc_next_addr32 :27;
	u32       Desc_next_addr64;
	u32       Desc_src_addr32;
	u32       Desc_src_addr64;
	u32       Desc_dest_addr32;
	u32       Desc_dest_addr64;
} DMA_TRANSFER_ELEMENT, *PDMA_TRANSFER_ELEMENT;


struct privData {
	struct pci_dev* pdev;          /**< PCI device entry */

	struct {
		u32 basePAddr;			/**< Base address of device memory */
		u32 baseLen;				/**< Length of device memory */
		void  __iomem* baseVAddr; /**< VA - mapped address */
	} barInfo[MAX_BARS];

	u32   HwErrCount;
	u32	count;
	int				Isr_Status;
	u32	dma_oper_length;
	u32	dma_allocate_len;
	u32	dma_bd_wcount;
	u32	dma_bd_Rcount;

	DmaRegisters  dmaregistes;
	u32 G5_SG_Dma_write_status;
	u32 G5_SG_Dma_Read_Status;
	u32 G5_CN_Dma_write_status;
	u32 G5_CN_Dma_Read_Status;
	u32 G5_SG_Dma_Write_Length;
	u32 G5_SG_Dma_Read_Length;
	u32 G5_CN_FDMA_Status;
	u32 G5_SG_DDR_Roffset;
	u32 G5_SG_DDR_Woffset;
	G5_DmaRegisters g5dmaregisters;
	char *vbuf;
	u32 *kbuf;


	//for isr count

	u32                   isr1_count;
	u32                   isr2_count;
	u32                   isr3_count;
	u32                   isr4_count;
	//G5_Dma_local_status g5dmalocalstatus;
};

struct sg_mapping_t {
	struct scatterlist *sgl;
	struct page **pages;
	int max_pages;
	int mapped_pages;
};
