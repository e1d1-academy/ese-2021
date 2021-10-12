#include <stdint.h>

#define REG(P) (*(volatile uint32_t *)(P))

#define RCU_APB2EN 0x40021018

#define GPIOA_BASE 0x40010800
#define CTL0_OFF 0x00
#define OCTL_OFF 0x0c

#define TIMER_BASE 0xd1000000
#define MTIME_OFF 0x0
#define MTIMECMP_OFF 0x8
#define RTC_FREQ 32768

// not used
//#define ECLIC_BASE 0xd2000000UL
//#define ECLIC_INTIPBASE ECLIC_BASE+0x1000
//#define ECLIC_INTIEBASE ECLIC_BASE+0x1001
//#define ECLIC_INTATBASE ECLIC_BASE+0x1002
//#define ECLIC_INTCTBASE ECLIC_BASE+0x1003

#define TIMER_INT 7

void set_timer1s()
{
	volatile uint64_t *mtime = (uint64_t *)(TIMER_BASE + MTIME_OFF);
	volatile uint64_t *mtimecmp = (uint64_t *)(TIMER_BASE + MTIMECMP_OFF);
	uint64_t now = *mtime;
	uint64_t then = now + 1000 * RTC_FREQ;
	*mtimecmp = then;
}

void irq_handler() __attribute__((interrupt, aligned(4)));
void irq_handler()
{
	// what's happening?
	unsigned long mcause_value = 0;
	asm volatile("csrr %0, mcause"
				 : "=r"(mcause_value));

	// take only interrupts
	if (mcause_value & 0x80000000)
	{
		// and only timer interrupts
		if ((mcause_value & 0x7fffffff) == 7)
		{
			// restart
			set_timer1s();
			// togle led
			REG(GPIOA_BASE + 0xc) ^= 2;
		}
	}
}

int main(void)
{
	// enable gpio port a clock
	REG(RCU_APB2EN) |= 0x4;

	// gpio a only port 1
	REG(GPIOA_BASE + CTL0_OFF) &= ~0xF0;
	// and set to output mode
	REG(GPIOA_BASE + CTL0_OFF) |= 0x30;

	// disable all interrupts
	asm volatile("csrci mstatus, 0x8");

	// timer-interrupt with 1 s
	set_timer1s();

	// set handler, take care of the lower six bits (see bumblebee doc 7.4.13)
	// set handler first, then bit(s) in mie
	asm volatile("csrw mtvec, %0" ::"r"(irq_handler));

	// only allow and enable MTIP
	asm volatile("csrw mie, %0" ::"r"(0x1 << TIMER_INT));

	// enable interrupts in general
	asm volatile("csrsi mstatus, 0x8");

	while (1)
	{
		asm volatile("wfi");
	}

	return 0;
}
