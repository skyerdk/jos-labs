// evil hello world -- kernel pointer passed to kernel
// kernel should destroy user environment in response

#include <inc/lib.h>
#include <inc/mmu.h>
#include <inc/x86.h>

char gdt_page[2 * PGSIZE];
void (*ring0_func)(void);
struct Gatedesc *target;
struct Gatedesc old_target;

// Call this function with ring0 privilege
void evil()
{
	// Kernel memory access
	*(char*)0xf010000a = 0;

	// Out put something via outb
	outb(0x3f8, 'I');
	outb(0x3f8, 'N');
	outb(0x3f8, ' ');
	outb(0x3f8, 'R');
	outb(0x3f8, 'I');
	outb(0x3f8, 'N');
	outb(0x3f8, 'G');
	outb(0x3f8, '0');
	outb(0x3f8, '!');
	outb(0x3f8, '!');
	outb(0x3f8, '!');
	outb(0x3f8, '\n');
}

static void
sgdt(struct Pseudodesc* gdtd)
{
	__asm __volatile("sgdt %0" :  "=m" (*gdtd));
}

void _call_wrapper()
{
	ring0_func();
	*target = old_target;
	__asm __volatile("leave\n\t"
		"lret");
}

// Invoke a given function pointer with ring0 privilege, then return to ring3
void ring0_call(void (*fun_ptr)(void)) {
	// Here's some hints on how to achieve this.
	// 1. Store the GDT descripter to memory (sgdt instruction)
	// 2. Map GDT in user space (sys_map_kernel_page)
	// 3. Setup a CALLGATE in GDT (SETCALLGATE macro)
	// 4. Enter ring0 (lcall instruction)
	// 5. Call the function pointer
	// 6. Recover GDT entry modified in step 3 (if any)
	// 7. Leave ring0 (lret instruction)

	// Hint : use a wrapper function to call fun_ptr. Feel free
	//        to add any functions or global variables in this
	//        file if necessary.

	// Lab3 : Your Code Here

	struct Pseudodesc gdt_pd;
	sgdt(&gdt_pd);

	if (sys_map_kernel_page((void *)gdt_pd.pd_base, gdt_page + PGSIZE) < 0)
		panic("ring0_call: sys_map_kernel_page failed\n");

	struct Segdesc *gdt = (struct Segdesc *)(ROUNDUP((void *)gdt_page, PGSIZE) + PGOFF(gdt_pd.pd_base));
	target = (struct Gatedesc *)gdt + (GD_TSS0 >> 3);
	old_target = *target;
	ring0_func = fun_ptr;
	SETCALLGATE(*target, GD_KT, _call_wrapper, 3);
	__asm __volatile("lcall %0, $0"
		:
		: "i"(GD_TSS0));
}

void
umain(int argc, char **argv)
{
	// call the evil function in ring0
	ring0_call(&evil);

	// call the evil function in ring3
	evil();
}
