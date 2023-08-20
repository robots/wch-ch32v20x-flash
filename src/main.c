
#include "platform.h"

#include "boot.h"

uint32_t SystemFrequency_APB1Clk = 56000000;

#define RCC_CFGR0_PLLMUL_Pos 18
#define RCC_CFGR0_PLLMUL_2   (0 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_3   (1 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_4   (2 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_5   (3 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_6   (4 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_7   (5 << RCC_CFGR0_PLLMUL_Pos)
#define RCC_CFGR0_PLLMUL_8   (6 << RCC_CFGR0_PLLMUL_Pos)

// set clock to 56MHz
void clock_init(void)
{
	// enable hse
	RCC->CTLR |= RCC_HSEON;

	// wait for hse rdy
	while((RCC->CTLR & RCC_HSERDY) == 0);

	// HSE not divided, HSE as source, no prescalers for APB1/2 AHB
	RCC->CFGR0 &= ~(RCC_PLLXTPRE | RCC_PLLSRC | RCC_HPRE_3 | RCC_PPRE1_2 | RCC_PPRE2_2);

	// 8Mhz * 7 = 56MHz
	RCC->CFGR0 |= RCC_CFGR0_PLLMUL_7;

	// enable pll
	RCC->CTLR |= RCC_PLLON;

	// wait for pll ready
	while((RCC->CTLR & RCC_PLLRDY) == 0);

	// select PLL as system clock
	RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_SW_0 | RCC_SW_1)) | RCC_SW_PLL;

	// wait for PLL selected as clock source
	while(RCC->CFGR0 & RCC_SWS != RCC_SWS_PLL);
}

// unlock flash
void flash_unlock(void)
{
	// enable flash interface
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xcdef89ab;
	for(volatile int i = 0; i < 100; i++);

	// enable extended flash interface
	FLASH->MODEKEYR = 0x45670123;
	FLASH->MODEKEYR = 0xcdef89ab;
	for(volatile int i = 0; i < 100; i++);
}

// program 256 bytes
// this function can program also bootloader area
void flash_boot_program256(uint32_t addr, uint8_t *buf, uint32_t size)
{
	// size needs to be max 256
	while(size > 256); // sanity check

	// start page program
	FLASH->CTLR |= 0x10000;
	while((FLASH->STATR & 1) != 0);

	// unlock bootloader area - 0x1fff8000
	FLASH->CTLR |= 0x20000000;

	// sanity check, these bits need to be set.
	while(FLASH->CTLR & 0x20010000 != 0x20010000);

	// program data 4 bytes at time
	uint32_t x = 0;
	do {
		*(uint32_t *)(addr + x) = *(uint32_t *)(buf + x);
		while((FLASH->STATR & 2) != 0);
		x += 4;
	} while (x < size);

	// start and wait
	FLASH->CTLR |= 0x200000;
	while((FLASH->STATR & 1) != 0);

	// clear done bit
	FLASH->STATR = 0x20;

	// clear enable bits
	FLASH->CTLR &= ~0x10000;
	FLASH->CTLR &= ~0x20000000;
}

// erase 4k page from bootloader area
// addr is address of page to erase
void flash_boot_erase4k(uint32_t addr)
{
	// erase 4k in bootloader area
	FLASH->CTLR = 0x40000002;

	// sanity check, these bits need to be set.
	while(FLASH->CTLR != 0x40000002);
 
	// set address
	FLASH->ADDR = addr;
	// start erase
	FLASH->CTLR |= 0x40;

	// wait for end
	do {

	} while((FLASH->STATR & 0x1) != 0);

	// clear done bit
	FLASH->STATR = 0x20;

	// clear enable bits
	FLASH->CTLR &= ~0x40000002;
}

void flash_boot_erase()
{
	uint32_t count4k = 7; // 7*4k = 28k area size
	for (uint32_t i = 0; i < count4k; i ++) {
		flash_boot_erase4k(0x1fff8000 + i * 0x1000);
	}
}

// program bootloader image
void flash_boot_prog(void)
{
	const uint32_t start_addr = 0x1fff8000;
	uint32_t buf[64];
	uint32_t left = boot_small_bin_size;
	uint32_t offset = 0;

	while (left > 0)
	{
		uint32_t tocpy = 0x100;
		if (tocpy > left) {
			tocpy = left;
		}

		memcpy(buf, &boot_small_bin[offset], tocpy);
		
		flash_boot_program256(start_addr + offset, (uint8_t *)buf, tocpy);

		left -= tocpy;
		offset += tocpy;
	}
}


// test writing of bootloader area
void flash_test(void)
{
	uint32_t start_addr = 0x1fff8000;
	uint32_t end_addr   = 0x1fff8000+0x7000;
	uint32_t buf[64];
	uint32_t addr = start_addr;

	while (addr < end_addr)
	{
		// fill test buffer
		for (uint32_t i = 0; i < 64; i ++)
		{
			buf[i] = addr + i*4;
		}

		flash_boot_program256(addr, (uint8_t *)buf, 0x100);
		addr += 0x100;
	}
}

void main(void)
{
	clock_init();

	// unlock flash
	flash_unlock();

	// erase boot area
	flash_boot_erase();

	// program bootloader image
	flash_boot_prog();

	while(1);
}
