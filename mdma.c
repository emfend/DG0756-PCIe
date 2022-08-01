/*******************************************************************************
 * (c) Copyright 2017-2018 Microsemi SoC Products Group. All rights reserved.
 *
 * File : mdma.c
 *
 * Description :  Polarfire PCIe Endpoint DMA specfic functionality
 *
********************************************************************************/

#include "mpcie.h"
#include "mhardware.h"
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/pagemap.h>

void mem_vfree(void *mem,unsigned long size)
{
	unsigned long addr;

	printk("in mem_vfree\n");

	size = PAGE_ALIGN(size);

	if (!mem)
		return;

	addr = (unsigned long)mem;
	while (size > 0) {
		ClearPageReserved(vmalloc_to_page((void *)addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	vfree(mem);
}
void *mem_vmalloc(unsigned long size)
{
	void *mem;
	unsigned long addr;

	printk("in mem_vmalloc\n");
	size = PAGE_ALIGN(size);
	mem = vmalloc_32(size);
	if (!mem)
		return NULL;

	addr = (unsigned long)mem;
	while (size > 0) {
		SetPageReserved(vmalloc_to_page((void *)addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	return mem;
}


int sgm_kernel_pages(struct sg_mapping_t *sg_map, char *start,u32 size)
{
  unsigned long first = ((unsigned long)start  & PAGE_MASK) >> PAGE_SHIFT;
  unsigned long last = (((unsigned long)start+size -1)  & PAGE_MASK) >> PAGE_SHIFT;
  u32 nr_pages = last-first +1;
  int i=0;
  struct scatterlist *sglist = sg_map->sgl;

  struct page **pages = sg_map->pages;
  unsigned char *virt = (unsigned char*)start;

  if(nr_pages > sg_map->max_pages)
  return -EINVAL;
  if(size == 0)
  return -EINVAL;

  sg_init_table(sglist,nr_pages);

  for(i=0;i<nr_pages;i++)
  {
    pages[i] = vmalloc_to_page(virt);
    if(pages[i] == NULL)
    {
      sg_map->mapped_pages = 0;
      return -ENOMEM;
    }
    virt +=PAGE_SIZE;
  }

  sg_map->mapped_pages = nr_pages;

  for(i=0;i<nr_pages;i++)
  {
    flush_dcache_page(pages[i]);
  }

  sg_set_page(&sglist[0],pages[0],0,offset_in_page(start));

  if(nr_pages > 1)
  {
    sglist[0].length = PAGE_SIZE - sglist[0].offset;
    size -= sglist[0].length;

    for(i=1;i<nr_pages - 1;i++)
    {
      sg_set_page(&sglist[i],pages[i],PAGE_SIZE,0);
      size -= PAGE_SIZE;
    }
    sg_set_page(&sglist[i],pages[i],size,0);

  }
  else{
    sglist[0].length = size;
  }
	printk("in sgm_kernel_pages  exit\n");
  return nr_pages;
}

void sg_destroy_mapper(struct sg_mapping_t *sg_map)
{

	printk("in sg_destroy_mapper\n");

	kfree(sg_map->sgl);
	kfree(sg_map->pages);
	kfree(sg_map);

}

struct sg_mapping_t* sg_list_create(u32 len)
{

  struct sg_mapping_t *sg_map;

	printk("in sg_list_create \n");

  sg_map = kcalloc(1,sizeof(struct sg_mapping_t),GFP_KERNEL);

  if(sg_map == NULL)
  return NULL;

  sg_map->max_pages = len / PAGE_SIZE + 2;

  sg_map->pages = kcalloc(sg_map->max_pages,sizeof(*sg_map->pages),GFP_KERNEL);
  if(sg_map->pages == NULL)
  {
    kfree(sg_map);
    return NULL;
  }

  sg_map->sgl = kcalloc(sg_map->max_pages,sizeof(struct scatterlist),GFP_KERNEL);
  if(sg_map->sgl == NULL)
  {
    kfree(sg_map->pages);
    kfree(sg_map);
    return NULL;
  }

  sg_init_table(sg_map->sgl,sg_map->max_pages);

  return sg_map;

}
