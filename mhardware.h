/*******************************************************************************
 * (c) Copyright 2017-2018 Microsemi SoC Products Group. All rights reserved.
 *
 * File : hardware.h
 *
 * Description : //Specific to hardware registers and design
 *
********************************************************************************/
// for
#define G5_ECC_DISABLE_OFFSET        0x280E //0xA038  //ECC disable
#define G5_SEC_ERROR_INT_OFFSET      0x280A //0xA028  //SEC_ERROR_INT  clear
#define G5_DED_ERROR_INT_OFFSET      0x280C //0xA030  //DED_ERROR_INT clear
#define G5_PCIE_EVENT_INT_OFFSET     0x2853 //0xA14c  //PCIE_EVENT_INT clear
#define G5_MSI_MASK_OFFSET           0x2062 //0x8188  //MSI MASK
#define G5_MSI_MASK_CLEAR            0x2063 //0x818c  //MSI MASK clear
#define G5_DMA_IMASK_OFFSET			 0x2060	//0x8180
#define G5_DMA_ISTATUS_LOCAL_OFFSET  0x2061 //0x8184  //ISTATUS_LOCAL
#define G5_DMA_IMASK_HOST_OFFSET	 0x2062 //0x8188  //IMASK_HOST
#define G5_DMA_ISTATUS_HOST			 0x2063 //0x818c ISTATUS_HOST

//For G5 DMA
#define G5_WDMA_SRCPARAM				0x2100  //0x8400
#define G5_WDMA_DESTPARAM				0x2101  //0x8404
#define G5_WDMA_SRCADDR32				0x2102  //0x8408
#define G5_WDMA_SRCADDR64				0x2103  //0x840c
#define G5_WDMA_DESTADDR32				0x2104  //0x8410
#define G5_WDMA_DESTADDR64				0x2105  //0x8414
#define G5_WDMA_LENGTH					0x2106  //0x8418
#define G5_WDMA_CONTROL					0x2107  //0x841c
#define G5_WDMA_STATUS					0x2108  //0x8420
#define G5_WDMA_PRC_LENGTH				0x2109  //0x8424
#define G5_WDMA_SHARE_ACCESS			0x210a  //0x8428

#define G5_RDMA_SRCPARAM				0x2110  //0x8440
#define G5_RDMA_DESTPARAM				0x2111  //0x8444
#define G5_RDMA_SRCADDR32				0x2112  //0x8448
#define G5_RDMA_SRCADDR64				0x2113  //0x844c
#define G5_RDMA_DESTADDR32				0x2114  //0x8450
#define G5_RDMA_DESTADDR64				0x2115  //0x8454
#define G5_RDMA_LENGTH					0x2116  //0x8458
#define G5_RDMA_CONTROL					0x2117  //0x845c
#define G5_RDMA_STATUS					0x2118  //0x8460
#define G5_RDMA_PRC_LENGTH				0x2119  //0x8440
#define G5_RDMA_SHARE_ACCESS			0x211a  //0x8440



#define G5_ECC_DISABLE_VAL			0x0F000000 //ECC disable
#define G5_SEC_ERROR_INT_VAL		0x0000FFFF //SEC_ERROR_INT  clear
#define G5_DED_ERROR_INT_VAL        0x0000FFFF //DED_ERROR_INT clear
#define G5_PCIE_EVENT_INT_VAL       0x00070007 //PCIE_EVENT_INT clear
#define G5_MSI_MASK_VAL				0xFF0000FF //MSI
#define G5_MSI_MASK_CLEAR_VAL       0xFFFFFFFF //0x818c  //MSI MASK clear


typedef struct {
	u32       Dma_Status : 8;
	u32	   Dma_Src_error : 8;
}G5_Dma_Status;


#define LED_GLOW_REG_OFFSET 0x8 /* Led Offset */
#define DIP_SWITCH_STATUS_REGISTER 0x10 /* Dip Switch Offset */
#define INTERRUPT_OFFSET 0x52

/* Device Serial Number Offset Value */
#define DSN_CTRL_OFFSET 0x28
#define DSN_CTRL_VALUE 0x100 /* Bit 8 set */
#define DSN_STATUS_OFFSET 0x24
#define DSN_STATUS_VALUE 0x4 /* Bit 4 set */

#define dsnOffset0 0x2010
#define dsnOffset1 0x2011
#define dsnOffset2 0x2012
#define dsnOffset3 0x2013
