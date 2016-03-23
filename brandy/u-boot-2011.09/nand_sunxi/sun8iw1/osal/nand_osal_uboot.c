/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include <stdarg.h>
#include <asm/arch/dma.h>
#include <sys_config.h>

#define get_wvalue(addr)	(*((volatile unsigned long  *)(addr)))
#define put_wvalue(addr, v)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

__u32 NAND_GetNdfcVersion(void);
void * NAND_Malloc(unsigned int Size);
void NAND_Free(void *pAddr, unsigned int Size);
static __u32 boot_mode;

int NAND_set_boot_mode(__u32 boot)
{
	boot_mode = boot;
	return 0;
}


int NAND_Print(const char * str, ...)
{
	if(boot_mode)
		return 0;
	else
	{
	    static char _buf[1024];
	    va_list args;

	    va_start(args, str);
	    vsprintf(_buf, str, args);

	    printf(_buf);
		return 0;
	}
    
}

__s32 NAND_CleanFlushDCacheRegion(__u32 buff_addr, __u32 len)
{
	flush_cache(buff_addr, len);
	return 0;
}

__u32 NAND_DMASingleMap(__u32 rw, __u32 buff_addr, __u32 len)
{
	return buff_addr;
}

__u32 NAND_DMASingleUnmap(__u32 rw, __u32 buff_addr, __u32 len)
{
	return buff_addr;
}

__s32 NAND_AllocMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{
	void *p = NULL;

#if 0

    __u32 physical_addr  = 0;
	//void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp);
	p = (void *)dma_alloc_coherent(NULL, PAGE_SIZE,
				(dma_addr_t *)&physical_addr, GFP_KERNEL);

	if (p == NULL) {
		printf("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = physical_addr;
		printf("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
			*cpu_addr, *phy_addr);
	}

#else

	p = (void *)NAND_Malloc(1024);

	if (p == NULL) {
		printf("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = (__u32)p;
		printf("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
			*cpu_addr, *phy_addr);
	}

#endif

	return 0;
}

__s32 NAND_FreeMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{

#if 0

	//void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle);
	dma_free_coherent(NULL, PAGE_SIZE, (void *)(*cpu_addr), *phy_addr);

#else

	NAND_Free((void *)(*cpu_addr), 1024);

#endif

	*cpu_addr = 0;
	*phy_addr = 0;

	return 0;
}

int NAND_WaitDmaFinish(void)
{
    return 0;
}

__u32 _Getpll6Clk(void)
{
	__u32 reg_val;
	__u32 factor_n;
	__u32 factor_k;
	//__u32 div_m;
	__u32 clock;

	reg_val  = *(volatile __u32 *)(0x01c20000 + 0x28);
	factor_n = ((reg_val >> 8) & 0x1f) + 1;
	factor_k = ((reg_val >> 4) & 0x3) + 1;
	//div_m = ((reg_val >> 0) & 0x3) + 1;

	clock = 24000000 * factor_n * factor_k/2;
	NAND_Print("pll6 clock is %d Hz\n", clock);
	if(clock != 600000000)
		printf("pll6 clock rate error, %d!!!!!!!\n", clock);

	return clock;
}

__s32 _get_ndfc_clk_v1(__u32 nand_index, __u32 *pdclk)
{
	__u32 sclk0_reg_adr;
	__u32 sclk_src, sclk_src_sel;
	__u32 sclk_pre_ratio_n, sclk_ratio_m;
	__u32 reg_val, sclk0;

	if (nand_index == 0) {
		sclk0_reg_adr = (0x01c20000 + 0x80); //CCM_NAND0_CLK0_REG;
	} else if (nand_index == 1) {
		sclk0_reg_adr = (0x01c20000 + 0x84); //CCM_NAND1_CLK0_REG;
	}
	else
	{
		printf("wrong nand id: %d\n", nand_index);
		return -1;
	}

	// sclk0
	reg_val = get_wvalue(sclk0_reg_adr);
	sclk_src_sel     = (reg_val>>24) & 0x3;
	sclk_pre_ratio_n = (reg_val>>16) & 0x3;;
	sclk_ratio_m     = (reg_val) & 0xf;
	if (sclk_src_sel == 0)
		sclk_src = 24;
	else
		sclk_src = _Getpll6Clk()/1000000;
	sclk0 = (sclk_src >> sclk_pre_ratio_n) / (sclk_ratio_m+1);

	if (nand_index == 0) {
		NAND_Print("get reg 0x01c20080: 0x%x\n", *(volatile __u32 *)(0x01c20080));
	} else {
		NAND_Print("get reg 0x01c20084: 0x%x\n", *(volatile __u32 *)(0x01c20084));
	}
	NAND_Print("get NDFC%d:  sclk0(2*dclk): %d MHz\n", nand_index, sclk0);

	*pdclk = sclk0/2;

	return 0;
}

__s32 _change_ndfc_clk_v1(__u32 nand_index, __u32 dclk_src_sel, __u32 dclk)
{
	u32 reg_val;
	u32 sclk0_src_sel, sclk0, sclk0_src, sclk0_pre_ratio_n, sclk0_src_t, sclk0_ratio_m;
	u32 sclk0_reg_adr;

	if (nand_index == 0) {
		sclk0_reg_adr = (0x01c20000 + 0x80); //CCM_NAND0_CLK0_REG;
	} else if (nand_index == 1) {
		sclk0_reg_adr = (0x01c20000 + 0x84); //CCM_NAND1_CLK0_REG;
	} else {
		printf("_change_ndfc_clk error, wrong nand index: %d\n", nand_index);
		return -1;
	}

	/*close dclk and cclk*/
	if (dclk == 0)
	{
		reg_val = get_wvalue(sclk0_reg_adr);
		reg_val &= (~(0x1U<<31));
		put_wvalue(sclk0_reg_adr, reg_val);

		printf("_change_ndfc_clk, close sclk0 and sclk!!!!\n");
		return 0;
	}

	sclk0_src_sel = dclk_src_sel;
	sclk0 = dclk*2; //set sclk0 to 2*dclk.

	if(sclk0_src_sel == 0x0) {
		//osc pll
        sclk0_src = 24;
	} else {
		//pll6 for ndfc version 1
		sclk0_src = _Getpll6Clk()/1000000;
	}

	//////////////////// sclk0: 2*dclk
	//sclk0_pre_ratio_n
	sclk0_pre_ratio_n = 3;
	if(sclk0_src > 4*16*sclk0)
		sclk0_pre_ratio_n = 3;
	else if (sclk0_src > 2*16*sclk0)
		sclk0_pre_ratio_n = 2;
	else if (sclk0_src > 1*16*sclk0)
		sclk0_pre_ratio_n = 1;
	else
		sclk0_pre_ratio_n = 0;

	sclk0_src_t = sclk0_src>>sclk0_pre_ratio_n;

	//sclk0_ratio_m
	sclk0_ratio_m = (sclk0_src_t/(sclk0)) - 1;
    if( sclk0_src_t%(sclk0) )
    	sclk0_ratio_m +=1;

	/////////////////////////////// close clock
	reg_val = get_wvalue(sclk0_reg_adr);
	reg_val &= (~(0x1U<<31));
	put_wvalue(sclk0_reg_adr, reg_val);

	///////////////////////////////configure
	//sclk0 <--> 2*dclk
	reg_val = get_wvalue(sclk0_reg_adr);
	//clock source select
	reg_val &= (~(0x3<<24));
	reg_val |= (sclk0_src_sel&0x3)<<24;
	//clock pre-divide ratio(N)
	reg_val &= (~(0x3<<16));
	reg_val |= (sclk0_pre_ratio_n&0x3)<<16;
	//clock divide ratio(M)
	reg_val &= ~(0xf<<0);
	reg_val |= (sclk0_ratio_m&0xf)<<0;
	put_wvalue(sclk0_reg_adr, reg_val);

	/////////////////////////////// open clock
	reg_val = get_wvalue(sclk0_reg_adr);
	reg_val |= 0x1U<<31;
	put_wvalue(sclk0_reg_adr, reg_val);

	NAND_Print("NAND_SetClk for nand index %d \n", nand_index);
	if (nand_index == 0) {
		NAND_Print("set reg 0x01c20080: 0x%x\n", *(volatile __u32 *)(0x01c20080));
	} else {
		NAND_Print("set reg 0x01c20084: 0x%x\n", *(volatile __u32 *)(0x01c20084));
	}

	return 0;
}

__s32 _open_ndfc_ahb_gate_and_reset_v1(__u32 nand_index)
{
	__u32 reg_val=0;

	/*
		1. release ahb reset and open ahb clock gate for ndfc version 1.
	*/
	if (nand_index == 0) {
		// ahb clock gate
		reg_val = *(volatile __u32 *)(0x01c20000 + 0x60);
		reg_val &= (~(0x1U<<13));
		reg_val |= (0x1U<<13);
		*(volatile __u32 *)(0x01c20000 + 0x60) = reg_val;

		// reset
		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val &= (~(0x1U<<13));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val |= (0x1U<<13);
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val &= (~(0x1U<<13));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val |= (0x1U<<13);
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

	} else if (nand_index == 1) {
		// ahb clock gate
		reg_val = *(volatile __u32 *)(0x01c20000 + 0x60);
		reg_val &= (~(0x1U<<12));
		reg_val |= (0x1U<<12);
		*(volatile __u32 *)(0x01c20000 + 0x60) = reg_val;

		// reset
		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val &= (~(0x1U<<12));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val |= (0x1U<<12);
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val &= (~(0x1U<<12));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

		reg_val = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		reg_val |= (0x1U<<12);
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;

	} else {
		printf("open ahb gate and reset, wrong nand index: %d\n", nand_index);
		return -1;
	}

	return 0;
}

__s32 _cfg_ndfc_gpio_v1(__u32 nand_index)
{
	__u32 cfg;

	if (nand_index == 0) {

		/* A31, A31s */
		//NAND_Print("set gpio for A31/A31s platform...ch0\n");

		*(volatile __u32 *)(0x01c20800 + 0x48) = 0x22222222;
		*(volatile __u32 *)(0x01c20800 + 0x4c) = 0x22222222;
		//*(volatile __u32 *)(0x01c20800 + 0x50) = 0x222;
		cfg = *(volatile __u32 *)(0x01c20800 + 0x54);
		cfg &= (~0xfff);
		cfg |= 0x222;
		*(volatile __u32 *)(0x01c20800 + 0x54) = cfg;
		NAND_Print("NAND_PIORequest, nand_index: 0x%x\n", nand_index);
		NAND_Print("Reg 0x01c20848: 0x%x\n", *(volatile __u32 *)(0x01c20848));
		NAND_Print("Reg 0x01c2084c: 0x%x\n", *(volatile __u32 *)(0x01c2084c));
		//NAND_Print("Reg 0x01c20850: 0x%x\n", *(volatile __u32 *)(0x01c20850));
		NAND_Print("Reg 0x01c20854: 0x%x\n", *(volatile __u32 *)(0x01c20854));
	} else if(nand_index == 1) {
		NAND_Print("Reg 0x01c20908: 0x%x\n", *(volatile __u32 *)(0x01c20908));
		*(volatile __u32 *)(0x01c20800 + 0x50) = 0x33333333;
		*(volatile __u32 *)(0x01c20800 + 0xfc) = 0x22222222;
		cfg = *(volatile __u32 *)(0x01c20800 + 0x100);
		cfg &= (~0xf);
		cfg |= (0x2);
		*(volatile __u32 *)(0x01c20800 + 0x100) = cfg;
		cfg = *(volatile __u32 *)(0x01c20800 + 0x108);
		cfg &= (~(0xff<<20));
		cfg |= (0x22<<20);
		*(volatile __u32 *)(0x01c20800 + 0x108) = cfg;

		NAND_Print("NAND_PIORequest, nand_index: 0x%x\n", nand_index);
		NAND_Print("Reg 0x01c20850: 0x%x\n", *(volatile __u32 *)(0x01c20850));
		NAND_Print("Reg 0x01c208fc: 0x%x\n", *(volatile __u32 *)(0x01c208fc));
		NAND_Print("Reg 0x01c20900: 0x%x\n", *(volatile __u32 *)(0x01c20900));
		NAND_Print("Reg 0x01c20908: 0x%x\n", *(volatile __u32 *)(0x01c20908));
	} else {
		printf("_cfg_ndfc_gpio_v1, wrong nand index %d\n", nand_index);
		return -1;
	}

	return 0;
}

int NAND_ClkRequest(__u32 nand_index)
{
	__s32 ret = 0;
	__u32 ndfc_version = NAND_GetNdfcVersion();

	if (ndfc_version == 1) {

		if (nand_index > 2) {
			printf("NAND_ClkRequest, wrong nand index %d for ndfc version %d\n",
					nand_index, ndfc_version);
			return -1;
		}
		// 1. release ahb reset and open ahb clock gate
		_open_ndfc_ahb_gate_and_reset_v1(nand_index);

		// 2. configure ndfc's sclk0
		ret = _change_ndfc_clk_v1(nand_index, 1, 10);
		if (ret<0) {
			printf("NAND_ClkRequest, set dclk failed!\n");
			return -1;
		}

	} else {
		printf("NAND_ClkRequest, wrong ndfc version, %d\n", ndfc_version);
		return -1;
	}

	return 0;
}

void NAND_ClkRelease(__u32 nand_index)
{
    return ;
}

/*
**********************************************************************************************************************
*
*             NAND_GetCmuClk
*
*  Description:
*
*
*  Parameters:
*
*
*  Return value:
*
*
**********************************************************************************************************************
*/
int NAND_SetClk(__u32 nand_index, __u32 nand_clk0, __u32 nand_clk1)
{
	__u32 ndfc_version = NAND_GetNdfcVersion();
	__u32 dclk_src_sel, dclk;
	__s32 ret = 0;

	if (ndfc_version == 1) {

		if (nand_index > 2) {
			printf("NAND_ClkRequest, wrong nand index %d for ndfc version %d\n",
					nand_index, ndfc_version);
			return -1;
		}

		////////////////////////////////////////////////
		dclk_src_sel = 1;
		dclk = nand_clk0;
		////////////////////////////////////////////////
		ret = _change_ndfc_clk_v1(nand_index, dclk_src_sel, dclk);
		if (ret < 0) {
			printf("NAND_SetClk, change ndfc clock failed\n");
			return -1;
		}

	} else {
		printf("NAND_SetClk, wrong ndfc version, %d\n", ndfc_version);
		return -1;
	}

	return 0;
}

int NAND_GetClk(__u32 nand_index, __u32 *pnand_clk0, __u32 *pnand_clk1)
{
	__s32 ret;
	__u32 ndfc_version = NAND_GetNdfcVersion();

	if (ndfc_version == 1) {

		NAND_Print("NAND_GetClk for nand index %d \n", nand_index);
		ret = _get_ndfc_clk_v1(nand_index, pnand_clk0);
		if (ret < 0) {
			printf("NAND_GetClk, failed!\n");
			return -1;
		}

	} else {
		printf("NAND_SetClk, wrong ndfc version, %d\n", ndfc_version);
		return -1;
	}

	return 0;
}

void NAND_PIORequest(__u32 nand_index)
{
	__s32 ret = 0;
	__u32 ndfc_version = NAND_GetNdfcVersion();

	if (ndfc_version == 1) {

		NAND_Print("NAND_PIORequest for nand index %d \n", nand_index);
		ret = _cfg_ndfc_gpio_v1(nand_index);
		if (ret < 0) {
			printf("NAND_PIORequest, failed!\n");
			return;
		}

	} else {
		printf("NAND_PIORequest, wrong ndfc version, %d\n", ndfc_version);
		return;
	}

	return;
}

__s32 NAND_PIOFuncChange_DQSc(__u32 nand_index, __u32 en)
{
	__u32 ndfc_version;
	__u32 cfg;

	ndfc_version = NAND_GetNdfcVersion();
	if (ndfc_version == 1) {
		printf("NAND_PIOFuncChange_EnDQScREc: invalid ndfc version!\n");
		return 0;
	}

	if (ndfc_version == 2) {

		if (nand_index == 0) {
			cfg = *(volatile __u32 *)(0x06000800 + 0x50);
			cfg &= (~(0x7U<<8));
			cfg |= (0x3U<<8);
			*(volatile __u32 *)(0x06000800 + 0x50) = cfg;
		} else {
			cfg = *(volatile __u32 *)(0x06000800 + 0x128);
			cfg &= (~(0x7U<<8));
			cfg |= (0x3U<<8);
			*(volatile __u32 *)(0x06000800 + 0x128) = cfg;
		}
	}

	return 0;
}
__s32 NAND_PIOFuncChange_REc(__u32 nand_index, __u32 en)
{
	__u32 ndfc_version;
	__u32 cfg;

	ndfc_version = NAND_GetNdfcVersion();
	if (ndfc_version == 1) {
		printf("NAND_PIOFuncChange_EnDQScREc: invalid ndfc version!\n");
		return 0;
	}

	if (ndfc_version == 2) {

		if (nand_index == 0) {
			cfg = *(volatile __u32 *)(0x06000800 + 0x50);
			cfg &= (~(0x7U<<4));
			cfg |= (0x3U<<4);
			*(volatile __u32 *)(0x06000800 + 0x50) = cfg;
		} else {
			cfg = *(volatile __u32 *)(0x06000800 + 0x128);
			cfg &= (~(0x7U<<4));
			cfg |= (0x3U<<4);
			*(volatile __u32 *)(0x06000800 + 0x128) = cfg;
		}
	}

	return 0;
}

void NAND_PIORelease(__u32 nand_index)
{
	return;
}

void NAND_Memset(void* pAddr, unsigned char value, unsigned int len)
{
    memset(pAddr, value, len);
}

void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len)
{
    memcpy(pAddr_dst, pAddr_src, len);
}

#if 0
#define NAND_MEM_BASE  0x59000000

void * NAND_Malloc(unsigned int Size)
{
	__u32 mem_addr;

	mem_addr = NAND_MEM_BASE+malloc_size;

	malloc_size += Size;
	if(malloc_size%4)
		malloc_size += (4-(malloc_size%4));

	//NAND_Print("NAND_Malloc: 0x%x\n", NAND_MEM_BASE + malloc_size);

	if(malloc_size>0x4000000)
		return NULL;
	else
		return (void *)mem_addr;
}

void NAND_Free(void *pAddr, unsigned int Size)
{
	//free(pAddr);
}

#else
void * NAND_Malloc(unsigned int Size)
{
	return malloc(Size);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
	free(pAddr);
}
#endif




void  OSAL_IrqUnLock(unsigned int  p)
{
    ;
}
void  OSAL_IrqLock  (unsigned int *p)
{
    ;
}

int NAND_WaitRbReady(void)
{
    return 0;
}

void *NAND_IORemap(unsigned int base_addr, unsigned int size)
{
    return (void *)base_addr;
}

__u32 NAND_VA_TO_PA(__u32 buff_addr)
{
    return buff_addr;
}

__u32 NAND_GetIOBaseAddrCH0(void)
{
	return 0x01c03000;
}

__u32 NAND_GetIOBaseAddrCH1(void)
{
	return 0x01c05000;
}

__u32 NAND_GetNdfcVersion(void)
{
	return 1;
}

__u32 NAND_GetNdfcDmaMode(void)
{
	/*
		0: General DMA;
		1: MBUS DMA

		Only support MBUS DMA!!!!
	*/
	return 1;
}

int NAND_PhysicLockInit(void)
{
    return 0;
}

int NAND_PhysicLock(void)
{
	return 0;
}

int NAND_PhysicUnLock(void)
{
	return 0;
}

int NAND_PhysicLockExit(void)
{
	return 0;
}

__u32 NAND_GetMaxChannelCnt(void)
{
	return 2;
}

__u32 NAND_GetPlatform(void)
{
	return 31;
}

unsigned int dma_chan = 0;

/* request dma channel and set callback function */
int nand_request_dma(void)
{
	printf("uboot nand_request_dma: current platform do not support general dma!\n");
	return -1;
}
int NAND_ReleaseDMA(__u32 nand_index)
{
	return 0;
}

int nand_dma_config_start(__u32 write, dma_addr_t addr,__u32 length)
{
	printf("uboot nand_dma_config_start: current platform do not support general dma!\n");
	return -1;
}

__u32 NAND_GetNandExtPara(__u32 para_num)
{
	int nand_para;
	script_parser_value_type_t ret;
	
	if (para_num == 0) {
		ret = script_parser_fetch("nand0_para", "nand_p0", &nand_para, 1);	
		if(ret!=SCRIPT_PARSER_OK)
		{
			printf("NAND_GetNandExtPara: get nand_p0 fail, %x\n", nand_para);
			return 0xffffffff;
		}
		else
			return nand_para;
	} else if (para_num == 1) {
		ret = script_parser_fetch("nand0_para", "nand_p1", &nand_para, 1);	
		if(ret!=SCRIPT_PARSER_OK)
		{
			printf("NAND_GetNandExtPara: get nand_p1 fail, %x\n", nand_para);
			return 0xffffffff;
		}
		else
			return nand_para;		
	} else {
		printf("NAND_GetNandExtPara: wrong para num: %d\n", para_num);
		return 0xffffffff;
	}
}

__u32 NAND_GetNandIDNumCtrl(void)
{
	int id_number_ctl;
	script_parser_value_type_t ret;

	ret = script_parser_fetch("nand0_para", "id_number_ctl", &id_number_ctl, 1);	
	if(ret!=SCRIPT_PARSER_OK) {
		printf("nand : get id_number_ctl fail, %x\n",id_number_ctl);
		return 0x0;
	} else {
		printf("nand : get id_number_ctl from script, %x\n",id_number_ctl);
		return id_number_ctl;
	}	
}

static void dumphex32(char *name, char *base, int len)
{
	__u32 i;

	if ((unsigned int)base&0xf0000000) {
		printf("dumphex32: err para in uboot, %s 0x%x\n", name, (unsigned int)base);
		return ;
	}
	
	printf("dump %s registers:", name);
	for (i=0; i<len*4; i+=4) {
		if (!(i&0xf))
			printf("\n0x%p : ", base + i);
		printf("0x%08x ", *((volatile unsigned int *)(base + i)));
	}
	printf("\n");
}

void NAND_DumpReg(void)
{
	dumphex32("nand0 reg", (char*)0x01c03000, 50);
	dumphex32("nand1 reg", (char*)0x01c05000, 50);
	dumphex32("gpio-c reg", (char*)0x01c20848, 10);
	dumphex32("gpio-h reg", (char*)0x01c208fc, 10);
	dumphex32("clock reg", (char*)0x01c20000, 8);
}

void NAND_ShowEnv(__u32 type, char *name, __u32 len, __u32 *val)
{
	int i;

    if (len && (val==NULL)) {
	printf("uboot:NAND_ShowEnv, para error!\n");
	return ;
    }

    if (type == 0)
    {
	printf("uboot:%s: ", name);
	for (i=0; i<len; i++)
	{
		if (i && (i%8==0))
			printf("\n");
		printf("%x ", val[i]);
	}
	printf("\n");
    }
    else
    {
	printf("uboot:NAND_ShowEnv, type error, %d!\n", type);
    }

    return ;
}

