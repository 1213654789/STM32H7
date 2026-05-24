#include "bsp_i2c_touch.h"

static uint32_t dwt_enabled = 0;

static void DWT_Init(void)
{
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    dwt_enabled = 1;
}

static void Delay_us(uint32_t us)
{
    if (!dwt_enabled) DWT_Init();
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

static void i2c_Delay(void)
{
    Delay_us(5);   // 调整至 5us，确保 I2C 时序稳定
}

static void I2C_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GTP_I2C_SCL_GPIO_CLK_ENABLE();
    GTP_I2C_SDA_GPIO_CLK_ENABLE();
    GTP_RST_GPIO_CLK_ENABLE();
    GTP_INT_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = GTP_I2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GTP_I2C_SCL_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GTP_I2C_SDA_PIN;
    HAL_GPIO_Init(GTP_I2C_SDA_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GTP_RST_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GTP_RST_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GTP_INT_GPIO_PIN;
    HAL_GPIO_Init(GTP_INT_GPIO_PORT, &GPIO_InitStruct);
}

void I2C_ResetChip(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GTP_INT_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GTP_INT_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GTP_INT_GPIO_PORT, GTP_INT_GPIO_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GTP_RST_GPIO_PORT, GTP_RST_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(GTP_RST_GPIO_PORT, GTP_RST_GPIO_PIN, GPIO_PIN_SET);
    HAL_Delay(55);

    GPIO_InitStruct.Pin = GTP_INT_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GTP_INT_GPIO_PORT, &GPIO_InitStruct);
}

void I2C_GTP_IRQEnable(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GTP_INT_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GTP_INT_GPIO_PORT, &GPIO_InitStruct);

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_NVIC_SetPriority(GTP_INT_EXTI_IRQ, 1, 1);
    HAL_NVIC_EnableIRQ(GTP_INT_EXTI_IRQ);
}

void I2C_GTP_IRQDisable(void)
{
    HAL_NVIC_DisableIRQ(GTP_INT_EXTI_IRQ);
}

void I2C_Touch_Init(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    DWT_Init();
    I2C_GPIO_Config();
    I2C_ResetChip();
    I2C_GTP_IRQDisable();
}

// 软件 I2C 时序函数 (与原逻辑一致，已用 HAL 宏)
static void i2c_Start(void)
{
    I2C_SDA_1(); I2C_SCL_1(); i2c_Delay();
    I2C_SDA_0(); i2c_Delay();
    I2C_SCL_0(); i2c_Delay();
}

static void i2c_Stop(void)
{
    I2C_SDA_0(); I2C_SCL_1(); i2c_Delay();
    I2C_SDA_1();
}

static void i2c_SendByte(uint8_t _ucByte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (_ucByte & 0x80) I2C_SDA_1();
        else I2C_SDA_0();
        i2c_Delay();
        I2C_SCL_1(); i2c_Delay();
        I2C_SCL_0();
        if (i == 7) I2C_SDA_1();
        _ucByte <<= 1;
        i2c_Delay();
    }
}

static uint8_t i2c_ReadByte(void)
{
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        value <<= 1;
        I2C_SCL_1(); i2c_Delay();
        if (I2C_SDA_READ()) value |= 1;
        I2C_SCL_0(); i2c_Delay();
    }
    return value;
}

static uint8_t i2c_WaitAck(void)
{
    I2C_SDA_1(); i2c_Delay();
    I2C_SCL_1(); i2c_Delay();
    if (I2C_SDA_READ()) { I2C_SCL_0(); i2c_Delay(); return 1; }
    I2C_SCL_0(); i2c_Delay();
    return 0;
}

static void i2c_Ack(void)
{
    I2C_SDA_0(); i2c_Delay();
    I2C_SCL_1(); i2c_Delay();
    I2C_SCL_0(); i2c_Delay();
    I2C_SDA_1();
}

static void i2c_NAck(void)
{
    I2C_SDA_1(); i2c_Delay();
    I2C_SCL_1(); i2c_Delay();
    I2C_SCL_0(); i2c_Delay();
}

#define I2C_DIR_WR 0
#define I2C_DIR_RD 1

uint32_t I2C_ReadBytes(uint8_t ClientAddr, uint8_t* pBuffer, uint16_t NumByteToRead)
{
    i2c_Start();
    i2c_SendByte(ClientAddr | I2C_DIR_RD);
    if (i2c_WaitAck() != 0) { i2c_Stop(); return 1; }
    while (NumByteToRead)
    {
        *pBuffer = i2c_ReadByte();
        if (NumByteToRead == 1) { i2c_NAck(); i2c_Stop(); }
        else i2c_Ack();
        pBuffer++; NumByteToRead--;
    }
    return 0;
}

uint32_t I2C_WriteBytes(uint8_t ClientAddr, uint8_t* pBuffer, uint8_t NumByteToWrite)
{
    uint16_t m;
    i2c_Stop();
    for (m = 0; m < 1000; m++)
    {
        i2c_Start();
        i2c_SendByte(ClientAddr | I2C_DIR_WR);
        if (i2c_WaitAck() == 0) break;
    }
    if (m == 1000) { i2c_Stop(); return 1; }
    while (NumByteToWrite--)
    {
        i2c_SendByte(*pBuffer);
        if (i2c_WaitAck() != 0) { i2c_Stop(); return 1; }
        pBuffer++;
    }
    i2c_Stop();
    return 0;
}