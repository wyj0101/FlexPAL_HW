#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>

#include "upgrade.h"
#include <zephyr/sys/sys_io.h>

/* STM32 系统存储器地址 */
#define STM32_BOOTLOADER_ADDR 0x1FFFF000UL

/**
 * @brief  手动硬编码 STM32F103 寄存器地址和位掩码
 */
#define RCC_BASE               0x40021000UL
#define RCC_CR                 (RCC_BASE + 0x00UL)
#define RCC_CFGR               (RCC_BASE + 0x04UL)
#define RCC_AHBRSTR            (RCC_BASE + 0x28UL)    // AHB外设复位寄存器
#define RCC_APB1RSTR           (RCC_BASE + 0x10UL)    // APB1外设复位寄存器  
#define RCC_APB2RSTR           (RCC_BASE + 0x0CUL)    // APB2外设复位寄存器
#define RCC_AHBENR             (RCC_BASE + 0x30UL)    // AHB外设时钟使能
#define RCC_APB1ENR            (RCC_BASE + 0x1CUL)    // APB1外设时钟使能
#define RCC_APB2ENR            (RCC_BASE + 0x18UL)    // APB2外设时钟使能
#define FLASH_ACR              0x40022000UL

// RCC_CR 位掩码
#define RCC_CR_HSION_BIT       (1UL << 0UL)
#define RCC_CR_HSIRDY_BIT      (1UL << 1UL)
#define RCC_CR_PLLON_BIT       (1UL << 24UL)

// RCC_CFGR 位掩码
#define RCC_CFGR_SW_HSI        (0UL << 0UL)
#define RCC_CFGR_SWS_MASK      (3UL << 2UL)
#define RCC_CFGR_SWS_HSI       (0UL << 2UL)
#define RCC_CFGR_HPRE_DIV1     (0UL << 4UL)
#define RCC_CFGR_PPRE1_DIV1    (0UL << 8UL)
#define RCC_CFGR_PPRE2_DIV1    (0UL << 11UL)
#define RCC_CFGR_HPRE_MASK     (0xFUL << 4UL)
#define RCC_CFGR_PPRE1_MASK    (0x7UL << 8UL)
#define RCC_CFGR_PPRE2_MASK    (0x7UL << 11UL)

// FLASH_ACR 位掩码
#define FLASH_ACR_LATENCY_MASK (0x7UL << 0UL)
#define FLASH_ACR_LATENCY_0WS  (0UL << 0UL)

/**
 * @brief  Zephyr环境下关闭所有中断
 */
__unused static void disable_all_interrupts(void)
{
    k_sched_lock();
    for (unsigned int irq = 0; irq < CONFIG_NUM_IRQS; irq++) {
        irq_disable(irq);
    }
    irq_lock();
}

/**
 * @brief  恢复USART1引脚为默认状态
 */
__unused static void restore_uart1_pins(void)
{
    const struct device *gpioa_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));
    if (device_is_ready(gpioa_dev)) {
        gpio_pin_configure(gpioa_dev, 9, GPIO_INPUT);
        gpio_pin_configure(gpioa_dev, 10, GPIO_INPUT);
    }
}

/**
 * @brief  切换时钟为HSI（8MHz）
 */
__unused static void switch_to_hsi_clock(void)
{
    // 1. 启用HSI时钟
    sys_write32(sys_read32(RCC_CR) | RCC_CR_HSION_BIT, RCC_CR);
    while ((sys_read32(RCC_CR) & RCC_CR_HSIRDY_BIT) == 0UL);

    // 2. 切换到HSI系统时钟
    sys_write32((sys_read32(RCC_CFGR) & ~(RCC_CFGR_SWS_MASK)) | RCC_CFGR_SW_HSI, RCC_CFGR);
    while ((sys_read32(RCC_CFGR) & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSI);

    // 3. 配置分频系数
    sys_write32((sys_read32(RCC_CFGR) & ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK))
                | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1,
                RCC_CFGR);

    // 4. 禁用PLL
    sys_write32(sys_read32(RCC_CR) & ~RCC_CR_PLLON_BIT, RCC_CR);

    // 5. 配置Flash等待周期
    sys_write32((sys_read32(FLASH_ACR) & ~FLASH_ACR_LATENCY_MASK) | FLASH_ACR_LATENCY_0WS, FLASH_ACR);
}

/**
 * @brief  复位所有外设
 */
__unused static void reset_all_peripherals(void)
{
    // 1. 复位所有外设
    sys_write32(0xFFFFFFFF, RCC_APB1RSTR);
    sys_write32(0xFFFFFFFF, RCC_APB2RSTR);
    sys_write32(0xFFFFFFFF, RCC_AHBRSTR);
    
    // 短暂延迟
    for(volatile int i = 0; i < 1000; i++);
    
    // 2. 清除复位标志
    sys_write32(0x00000000, RCC_APB1RSTR);
    sys_write32(0x00000000, RCC_APB2RSTR);
    sys_write32(0x00000000, RCC_AHBRSTR);
    
    // 3. 禁用所有外设时钟
    sys_write32(0x00000000, RCC_APB1ENR);
    sys_write32(0x00000000, RCC_APB2ENR);
    sys_write32(0x00000000, RCC_AHBENR);
}

/**
 * @brief  核心跳转函数：从Zephyr App跳转到STM32内置Bootloader
 */
/*
void jump_to_bootloader(void)
{
    // 1. 关闭所有中断和Zephyr调度
    disable_all_interrupts();

    // 2. 切换时钟为HSI（Bootloader兼容的时钟）
    // switch_to_hsi_clock();

    // 3. 复位所有外设
    reset_all_peripherals();

    // 4. 恢复USART1引脚为默认状态
    restore_uart1_pins();

    // 5. 读取Bootloader向量表
    uint32_t bootloader_stack = *(volatile uint32_t *)STM32_BOOTLOADER_ADDR;
    uint32_t bootloader_entry = *(volatile uint32_t *)(STM32_BOOTLOADER_ADDR + 4);

     uint32_t safe_stack = 0x20002800;

    // 6. 直接跳转到Bootloader
    __asm__ volatile (
        "mov sp, %0\n"    // 设置栈指针
        "bx %1"           // 直接跳转到入口地址
        : 
        : "r" (safe_stack), "r" (bootloader_entry)
    );

    // 不会执行到这里
    while (1);
}
*/
void jump_to_bootloader(void)
{
    // 读取Bootloader入口地址
    uint32_t bootloader_entry = *(volatile uint32_t *)(0x1FFFF004);
    
    __asm__ volatile (
        // 1. 关闭所有中断（PRIMASK）
        "cpsid i\n"
        
        // 2. 清理特殊功能寄存器
        "mov r0, #0\n"
        "msr control, r0\n"        // 回到特权模式，使用MSP
        "msr psp, r0\n"            // 清理进程栈指针
        "msr basepri, r0\n"        // 清除基础优先级
        
        // 3. 切换回HSI时钟（关键步骤）
        "ldr r1, =0x40021000\n"    // RCC_BASE
        
        // 禁用PLL
        "ldr r0, [r1, #0x00]\n"    // RCC_CR
        "bic r0, r0, #0x01000000\n" // 清除PLLON位 (bit 24)
        "str r0, [r1, #0x00]\n"
        
        // 启用HSI
        "ldr r0, [r1, #0x00]\n"
        "orr r0, r0, #0x00000001\n" // 设置HSION位 (bit 0)
        "str r0, [r1, #0x00]\n"
        
        // 等待HSI就绪
        "1:\n"
        "ldr r0, [r1, #0x00]\n"
        "ands r0, r0, #0x00000002\n" // 检查HSIRDY位 (bit 1)
        "beq 1b\n"
        
        // 切换到HSI系统时钟
        "ldr r0, [r1, #0x04]\n"    // RCC_CFGR
        "bic r0, r0, #0x00000003\n" // 清除SW[1:0]
        "str r0, [r1, #0x04]\n"     // SW = HSI
        
        // 等待时钟切换完成
        "2:\n"
        "ldr r0, [r1, #0x04]\n"
        "ands r0, r0, #0x0000000C\n" // 检查SWS[1:0]
        "bne 2b\n"
        
        // 4. 禁用所有外设时钟
        "mov r0, #0\n"
        "str r0, [r1, #0x1C]\n"    // RCC_APB1ENR = 0
        "str r0, [r1, #0x18]\n"    // RCC_APB2ENR = 0
        "str r0, [r1, #0x30]\n"    // RCC_AHBENR = 0
        
        // 5. 复位所有外设
        "mov r0, #0xFFFFFFFF\n"
        "str r0, [r1, #0x10]\n"    // RCC_APB1RSTR
        "str r0, [r1, #0x0C]\n"    // RCC_APB2RSTR
        "str r0, [r1, #0x28]\n"    // RCC_AHBRSTR
        
        // 短暂延时
        "mov r2, #1000\n"
        "3:\n"
        "subs r2, r2, #1\n"
        "bne 3b\n"
        
        // 清除复位
        "mov r0, #0\n"
        "str r0, [r1, #0x10]\n"    // RCC_APB1RSTR = 0
        "str r0, [r1, #0x0C]\n"    // RCC_APB2RSTR = 0
        "str r0, [r1, #0x28]\n"    // RCC_AHBRSTR = 0
        
        // 6. 配置Flash等待周期（8MHz下为0等待）
        "ldr r1, =0x40022000\n"    // FLASH_ACR
        "mov r0, #0x00\n"          // LATENCY = 0
        "str r0, [r1]\n"
        
        // 7. 清理寄存器状态
        "mov r0, #0\n"
        "mov r1, #0\n"
        "mov r2, #0\n"
        "mov r3, #0\n"
        "mov r4, #0\n"
        "mov r5, #0\n"
        "mov r6, #0\n"
        "mov r7, #0\n"
        "mov r8, #0\n"
        "mov r9, #0\n"
        "mov r10, #0\n"
        "mov r11, #0\n"
        "mov r12, #0\n"
        
        // 8. 内存屏障
        "dsb\n"
        "isb\n"
        
        // 9. 直接跳转到Bootloader（不设置栈，让Bootloader自己设置）
        "mov pc, %0\n"
        
        : 
        : "r" (bootloader_entry)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "memory"
    );
}