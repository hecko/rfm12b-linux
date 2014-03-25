// iolib.c
// Simple I/O library
// v1 October 2013 - shabaz
// v2 March 2014 - Daniel Stelter-Gliese (adjustments to make it run in kernel mode)


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/io.h>

#include "rfm12b_config.h"
#include "iolib.h"

#pragma GCC diagnostic ignored "-Wchar-subscripts"

const unsigned int ioregion_base[]={GPIO0, GPIO1, GPIO2, GPIO3};

const char p8_bank[]={-1,-1, 1, 1, 1, 1, 2, 2, 
                       2, 2, 1, 1, 0, 0, 1, 1, 
                       0, 2, 0, 1, 1, 1, 1, 1,
                       1, 1, 2, 2, 2, 2, 0, 0,  
                       0, 2, 0, 2, 2, 2, 2,-1,
                       2, 2, 2, 2, 2, 2};
                 
const unsigned int p8_bitmask[]={
	     0,     0,  1<<6,  1<<7,  1<<2,  1<<3,  1<<2,  1<<3, 
    1<<5,  1<<4, 1<<13, 1<<12, 1<<23, 1<<26, 1<<15, 1<<14,
    1<<27,  1<<1, 1<<22, 1<<31, 1<<30,  1<<5,  1<<4,  1<<1,
    1<<0, 1<<29, 1<<22, 1<<24, 1<<23, 1<<25, 1<<10, 1<<11,
    1<<9, 1<<17,  1<<8, 1<<16, 1<<14, 1<<15, 1<<12,     0,
    1<<10, 1<<11,  1<<8,  1<<9,  1<<6,  1<<7};

const char p9_bank[]={-1,-1,-1,-1,-1,-1,-1,-1,
	                    -1,-1, 0, 1, 0, 1, 1, 1, 
	                     0, 0, 0, 0, 0, 0, 1,-1, 
	                     3,-1, 3, 3, 3,-1, 3,-1,
	                    -1,-1,-1,-1,-1,-1,-1,-1,
	                     0, 0,-1,-1,-1,-1};
	                    
const unsigned int p9_bitmask[]={
	     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0, 1<<30, 1<<28, 1<<31, 1<<18, 1<<16, 1<<19, 
    1<<5,  1<<4, 1<<13, 1<<12,  1<<3,  1<<2, 1<<17,     0,
   1<<21,     0, 1<<19, 1<<17, 1<<15,     0, 1<<14,     0,
       0,     0,     0,     0,     0,     0,     0,     0,
   1<<20,  1<<7,     0,     0,     0,     0};
	                      
volatile unsigned int *gpio_addr[4]={NULL, NULL, NULL, NULL};
volatile unsigned int *ctrl_addr=NULL;
char* bank[2];
unsigned int* port_bitmask[2];

int
iolib_init(void)
{
	int i;
	
	bank[0]=(char*)p8_bank;
	bank[1]=(char*)p9_bank;
	port_bitmask[0]=(unsigned int*)p8_bitmask;
	port_bitmask[1]=(unsigned int*)p9_bitmask;
	
	for (i=0; i<4; i++)
	{
		gpio_addr[i] = ioremap(ioregion_base[i], GPIOX_LEN);
		printk(KERN_INFO RFM12B_DRV_NAME" : ioremap(0x%08x) = 0x%p\n", ioregion_base[i], gpio_addr[i]);
	}
	return(0);
}

int
iolib_free(void)
{
	int i;
	for (i=0; i<4; i++) {
		if (gpio_addr[i]) {
			iounmap(gpio_addr[i]);
			gpio_addr[i] = NULL;
		}
	}
	return(0);
}

int
iolib_setdir(char port, char pin, char dir)
{
	//int i;
	int param_error=0;
	volatile unsigned int* reg;
	
	// sanity checks
	if (gpio_addr[0]==0)
		param_error=1;
	if ((port<8) || (port>>9))
		param_error=1;
	if ((pin<1) || (pin>46))
		param_error=1;
	if (bank[port][pin]<0)
		param_error=1;
	if (param_error)
	{
		printk("iolib_setdir: parameter error!\n");
		return(-1);
	}
	
	reg=(void*)gpio_addr[bank[port-8][pin-1]]+GPIO_OE;
	
	if (dir==DIR_OUT)
	{
	  unsigned int v = readl(reg);
	  v &= ~(port_bitmask[port-8][pin-1]);
	  writel(v, reg);
	}
	else if (dir==DIR_IN)
	{
		unsigned int v = readl(reg);
		v |= port_bitmask[port-8][pin-1];
		writel(v, reg);
	}
	
	return(0);
}

inline void
pin_high(char port, char pin)
{
  void* ptr = ((void *)gpio_addr[bank[port-8][pin-1]]+GPIO_SETDATAOUT);
  printk(KERN_INFO RFM12B_DRV_NAME" : pin_high(%p)\n", ptr);
  writel(port_bitmask[port-8][pin-1], ptr);
}

inline void
pin_low(char port, char pin)
{
  void* ptr = ((void *)gpio_addr[bank[port-8][pin-1]]+GPIO_CLEARDATAOUT);
  printk(KERN_INFO RFM12B_DRV_NAME" : pin_low(%p)\n", ptr);
  writel(port_bitmask[port-8][pin-1], ptr);
}

inline char
is_high(char port, char pin)
{
	unsigned int v = readl((void *)gpio_addr[bank[port-8][pin-1]]+GPIO_DATAIN);
	return (v & port_bitmask[port-8][pin-1]) != 0;
}

inline char
is_low(char port, char pin)
{
	unsigned int v = readl((void *)gpio_addr[bank[port-8][pin-1]]+GPIO_DATAIN);
	return (v & port_bitmask[port-8][pin-1]) == 0;
}

