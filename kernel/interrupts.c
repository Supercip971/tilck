
#include <common/basic_defs.h>
#include <common/string_util.h>
#include <exos/process.h>
#include <exos/hal.h>

void handle_syscall(regs *);
void handle_fault(regs *);
void handle_irq(regs *r);

extern volatile u64 jiffies;

volatile int nested_interrupts_count = 0;
volatile int nested_interrupts[32] = { [0 ... 31] = -1 };

extern inline bool in_syscall(void);
extern inline int get_curr_interrupt(void);

void check_not_in_irq_handler(void)
{
   uptr var;

   if (!in_panic) {
      disable_interrupts(&var);
      {
         if (nested_interrupts_count > 0)
            if (is_irq(nested_interrupts[nested_interrupts_count - 1]))
               panic("NOT expected to be in an interrupt handler.");
      }
      enable_interrupts(&var);
   }
}

int get_curr_interrupt(void)
{
   ASSERT(!are_interrupts_enabled());
   return nested_interrupts[nested_interrupts_count - 1];
}

/*
 * TODO: re-implement this when SYSENTER is supported as well
 */
bool in_syscall(void)
{
   uptr var;
   bool res = false;
   disable_interrupts(&var);

   for (int i = nested_interrupts_count - 1; i >= 0; i--) {
      if (nested_interrupts[i] == 0x80) {
         res = true;
         break;
      }
   }

   enable_interrupts(&var);
   return res;
}

static bool is_same_interrupt_nested(int int_num)
{
   ASSERT(!are_interrupts_enabled());

   for (int i = nested_interrupts_count - 1; i >= 0; i--)
      if (nested_interrupts[i] == int_num)
         return true;

   return false;
}

/*
 * This sanity check is very fundamental: it assures us that in no case
 * we're running an usermode thread with preemption disabled.
 */
static ALWAYS_INLINE void DEBUG_check_preemption_enabled_for_usermode()
{
   if (current && !running_in_kernel(current) && !nested_interrupts_count) {
      ASSERT(is_preemption_enabled());
   }
}

void generic_interrupt_handler(regs *r)
{
   ASSERT(!are_interrupts_enabled());
   DEBUG_VALIDATE_STACK_PTR();
   ASSERT(!is_same_interrupt_nested(regs_intnum(r)));
   ASSERT(current != NULL);

   DEBUG_check_preemption_enabled_for_usermode();

   if (is_irq(regs_intnum(r))) {

      handle_irq(r);
      DEBUG_check_preemption_enabled_for_usermode();
      return;
   }

   disable_preemption();
   push_nested_interrupt(regs_intnum(r));

   enable_interrupts_forced();
   ASSERT(are_interrupts_enabled());

   if (LIKELY(regs_intnum(r) == SYSCALL_SOFT_INTERRUPT)) {
      handle_syscall(r);
   } else {
      VERIFY(is_fault(regs_intnum(r)));
      handle_fault(r);
   }

   enable_preemption();

   DEBUG_check_preemption_enabled_for_usermode();
   pop_nested_interrupt();
}

