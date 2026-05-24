#ifndef __BSP_I2C_TOUCH_H
#define __BSP_I2C_TOUCH_H

#include "stm32h7xx_hal.h"

/* 设备地址 */
#define GTP_ADDRESS             0xBA

/*
RST 引脚连接到 GPIOA7 引脚
INT 引脚连接到 GPIOD15 引脚
SCL 引脚连接到 GPIOD1 引脚
SDA 引脚连接到 GPIOE10 引脚
*/


/* ---------- I2C 引脚 ---------- */
#define GTP_I2C_SCL_PIN                  GPIO_PIN_1
#define GTP_I2C_SCL_GPIO_PORT            GPIOD
#define GTP_I2C_SCL_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOD_CLK_ENABLE()

#define GTP_I2C_SDA_PIN                  GPIO_PIN_10
#define GTP_I2C_SDA_GPIO_PORT            GPIOE
#define GTP_I2C_SDA_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOE_CLK_ENABLE()

/* ---------- 复位引脚 ---------- */
#define GTP_RST_GPIO_PORT                GPIOA
#define GTP_RST_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()
#define GTP_RST_GPIO_PIN                 GPIO_PIN_7

/* ---------- 中断引脚 ---------- */
#define GTP_INT_GPIO_PORT                GPIOD
#define GTP_INT_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOD_CLK_ENABLE()
#define GTP_INT_GPIO_PIN                 GPIO_PIN_15
#define GTP_INT_EXTI_LINE                EXTI_LINE_15
#define GTP_INT_EXTI_IRQ                 EXTI15_10_IRQn
#define GTP_INT_IRQHandler               EXTI15_10_IRQHandler

/* ---------- 软件 I2C 操作宏 (HAL) ---------- */
#define I2C_SCL_1()  HAL_GPIO_WritePin(GTP_I2C_SCL_GPIO_PORT, GTP_I2C_SCL_PIN, GPIO_PIN_SET)
#define I2C_SCL_0()  HAL_GPIO_WritePin(GTP_I2C_SCL_GPIO_PORT, GTP_I2C_SCL_PIN, GPIO_PIN_RESET)
#define I2C_SDA_1()  HAL_GPIO_WritePin(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN, GPIO_PIN_SET)
#define I2C_SDA_0()  HAL_GPIO_WritePin(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN, GPIO_PIN_RESET)
#define I2C_SDA_READ()  (HAL_GPIO_ReadPin(GTP_I2C_SDA_GPIO_PORT, GTP_I2C_SDA_PIN) == GPIO_PIN_SET)

/* ---------- 函数声明 ---------- */
void I2C_Touch_Init(void);
uint32_t I2C_WriteBytes(uint8_t ClientAddr, uint8_t* pBuffer, uint8_t NumByteToWrite);
uint32_t I2C_ReadBytes(uint8_t ClientAddr, uint8_t* pBuffer, uint16_t NumByteToRead);
void I2C_ResetChip(void);
void I2C_GTP_IRQEnable(void);
void I2C_GTP_IRQDisable(void);

#endif