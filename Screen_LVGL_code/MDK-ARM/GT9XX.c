#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "GT9xx.h"
#include "bsp_i2c_touch.h"    /* 已移植的软件 I2C 驱动 */

/* 串口调试输出实现 */
void GTP_UART_Printf(const char *prefix, const char *fmt, ...)
{
    static char buf[128];
    va_list args;
    int len;
    
    va_start(args, fmt);
    /* 添加前缀 */
    len = sprintf(buf, "%s", prefix);
    /* 添加格式化内容 */
    len += vsprintf(&buf[len], fmt, args);
    /* 添加换行 */
    len += sprintf(&buf[len], "\r\n");
    va_end(args);
    
    if (len > 0 && len < 128) {
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)buf, len, 0xFFFF);
    }
}

/* 7寸屏 GT917S 驱动配置（可根据实际屏幕微调） */
const uint8_t CTP_CFG_GT917S[] = {
  0x84,0x20,0x03,0xE0,0x01,0x05,0x05,0x00,0x00,0x40,
  0x00,0x0F,0x78,0x64,0x53,0x11,0x00,0x00,0x00,0x00,
  0x23,0x17,0x19,0x1D,0x0F,0x04,0x00,0x00,0x00,0x00,
  0x00,0x00,0x04,0x51,0x14,0x00,0x00,0x00,0x00,0x00,
  0x32,0x00,0x00,0x50,0x38,0x28,0x8A,0x20,0x11,0x37,
  0x39,0xA2,0x07,0x38,0x6D,0x28,0x11,0x03,0x24,0x00,
  0x01,0x28,0x50,0xC0,0x94,0x02,0x00,0x00,0x53,0xB8,
  0x2E,0xA2,0x35,0x8F,0x3B,0x80,0x42,0x75,0x49,0x6B,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x4C,0x3C,
  0xFF,0xFF,0x07,0x14,0x14,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x73,
  0x50,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x1F,0x1D,0x1B,0x1A,0x19,0x18,0x17,0x16,0x15,0x09,
  0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0x1C,0x1B,0x1A,0x19,0x18,0x17,0x15,0x14,
  0x13,0x12,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x05,0x00,0x00,0x0F,
  0x00,0x00,0x00,0x80,0x46,0x08,0x96,0x50,0x32,0x0A,
  0x0A,0x64,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x32,0x03,0x0C,0x08,0x23,0x00,0x14,0x23,0x00,0x28,
  0x46,0x30,0x3C,0xD0,0x07,0x50,0x70,0xB0,0x01
};

TOUCH_IC touchIC = GT917S;

const TOUCH_PARAM_TypeDef touch_param[1] = {
  /* GT917S, 4.3寸屏 480x272 */
  {
    .max_width = 480,
    .max_height = 272,
    .config_reg_addr = 0x8050,
  }
};

/* 内部函数声明 */
static int8_t GTP_I2C_Test(void);
static int I2C_Transfer(struct i2c_msg *msgs, int num);
static int32_t GTP_I2C_Read(uint8_t client_addr, uint8_t *buf, int32_t len);
static int32_t GTP_I2C_Write(uint8_t client_addr, uint8_t *buf, int32_t len);
static void Goodix_TS_Work_Func(void);
static void GTP_Touch_Down(int32_t id, int32_t x, int32_t y, int32_t w);
static void GTP_Touch_Up(int32_t id);

/* ---------------- I2C 传输函数 ---------------- */
static int I2C_Transfer(struct i2c_msg *msgs, int num)
{
    int im = 0;
    int ret = 0;
    for (im = 0; ret == 0 && im != num; im++)
    {
        if (msgs[im].flags & I2C_M_RD)
            ret = I2C_ReadBytes(msgs[im].addr, msgs[im].buf, msgs[im].len);
        else
            ret = I2C_WriteBytes(msgs[im].addr, msgs[im].buf, msgs[im].len);
    }
    if (ret) return ret;
    return im;
}

static int32_t GTP_I2C_Read(uint8_t client_addr, uint8_t *buf, int32_t len)
{
    struct i2c_msg msgs[2];
    int32_t ret = -1;
    int32_t retries = 0;

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client_addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client_addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

    while (retries < 5)
    {
        ret = I2C_Transfer(msgs, 2);
        if (ret == 2) break;
        retries++;
    }
    if (retries >= 5)
        GTP_ERROR("I2C Read: 0x%04X, %d bytes failed.", ((buf[0]<<8)|buf[1]), len-2);
    return ret;
}

static int32_t GTP_I2C_Write(uint8_t client_addr, uint8_t *buf, int32_t len)
{
    struct i2c_msg msg;
    int32_t ret = -1;
    int32_t retries = 0;

    msg.flags = !I2C_M_RD;
    msg.addr  = client_addr;
    msg.len   = len;
    msg.buf   = buf;

    while (retries < 5)
    {
        ret = I2C_Transfer(&msg, 1);
        if (ret == 1) break;
        retries++;
    }
    if (retries >= 5)
        GTP_ERROR("I2C Write: 0x%04X, %d bytes failed.", ((buf[0]<<8)|buf[1]), len-2);
    return ret;
}

int32_t GTP_I2C_Read_dbl_check(uint8_t client_addr, uint16_t addr, uint8_t *rxbuf, int len)
{
    uint8_t buf[16] = {0};
    uint8_t confirm_buf[16] = {0};
    uint8_t retry = 0;

    while (retry++ < 3)
    {
        memset(buf, 0xAA, 16);
        buf[0] = (uint8_t)(addr >> 8);
        buf[1] = (uint8_t)(addr & 0xFF);
        GTP_I2C_Read(client_addr, buf, len + 2);

        memset(confirm_buf, 0xAB, 16);
        confirm_buf[0] = (uint8_t)(addr >> 8);
        confirm_buf[1] = (uint8_t)(addr & 0xFF);
        GTP_I2C_Read(client_addr, confirm_buf, len + 2);

        if (!memcmp(buf, confirm_buf, len+2))
        {
            memcpy(rxbuf, confirm_buf+2, len);
            return SUCCESS;
        }
    }
    GTP_ERROR("I2C read 0x%04X, %d bytes, double check failed!", addr, len);
    return FAIL;
}

/* ---------------- 中断控制 ---------------- */
void GTP_IRQ_Disable(void)
{
    I2C_GTP_IRQDisable();  /* 调用软件 I2C 中断屏蔽 */
}

void GTP_IRQ_Enable(void)
{
    I2C_GTP_IRQEnable();
}

/* ---------------- 触摸处理 ---------------- */
volatile static int16_t pre_x[GTP_MAX_TOUCH] = {-1,-1,-1,-1,-1};
volatile static int16_t pre_y[GTP_MAX_TOUCH] = {-1,-1,-1,-1,-1};

static void GTP_Touch_Down(int32_t id, int32_t x, int32_t y, int32_t w)
{
    GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);
    pre_x[id] = x;
    pre_y[id] = y;
    /* 在此添加触摸按下处理 */
}

static void GTP_Touch_Up(int32_t id)
{
    pre_x[id] = -1;
    pre_y[id] = -1;
    GTP_DEBUG("Touch id[%2d] release!", id);
}

static void Goodix_TS_Work_Func(void)
{
    uint8_t  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    uint8_t  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    uint8_t  touch_num = 0;
    uint8_t  finger = 0;
    uint8_t  client_addr = GTP_ADDRESS;
    uint8_t  *coor_data = NULL;
    int32_t  input_x = 0, input_y = 0, input_w = 0;
    uint8_t  id = 0;
    uint8_t  pre_id[GTP_MAX_TOUCH] = {0};
    static uint16_t pre_touch = 0;

    if (GTP_I2C_Read(client_addr, point_data, 12) < 0) return;

    finger = point_data[GTP_ADDR_LENGTH];
    if (finger == 0x00) return;
    if ((finger & 0x80) == 0) goto exit_work_func;

    touch_num = finger & 0x0f;
    if (touch_num > GTP_MAX_TOUCH) goto exit_work_func;

    if (touch_num > 1)
    {
        uint8_t buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};
        GTP_I2C_Read(client_addr, buf, 2 + 8 * (touch_num - 1));
        memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
    }

    if (pre_touch > touch_num)
    {
        for (int i = 0; i < pre_touch; i++)
        {
            uint8_t j;
            for (j = 0; j < touch_num; j++)
            {
                coor_data = &point_data[j * 8 + 3];
                id = coor_data[0] & 0x0F;
                if (pre_id[i] == id) break;
            }
            if (j >= touch_num)
                GTP_Touch_Up(pre_id[i]);
        }
    }

    if (touch_num)
    {
        for (int i = 0; i < touch_num; i++)
        {
            coor_data = &point_data[i * 8 + 3];
            id = coor_data[0] & 0x0F;
            pre_id[i] = id;
            input_x = coor_data[1] | (coor_data[2] << 8);
            input_y = coor_data[3] | (coor_data[4] << 8);
            input_w = coor_data[5] | (coor_data[6] << 8);
            GTP_Touch_Down(id, input_x, input_y, input_w);
        }
    }
    else if (pre_touch)
    {
        for (int i = 0; i < pre_touch; i++)
            GTP_Touch_Up(pre_id[i]);
    }

    pre_touch = touch_num;

exit_work_func:
    GTP_I2C_Write(client_addr, end_cmd, 3);
}

void GTP_TouchProcess(void)
{
    GTP_DEBUG_FUNC();
    Goodix_TS_Work_Func();
}

/* ---------------- 复位 ---------------- */
int8_t GTP_Reset_Guitar(void)
{
    I2C_ResetChip();   /* 已在 bsp_i2c_touch.c 中实现 */
    return 0;
}

/* ---------------- 获取信息 ---------------- */
static int32_t GTP_Get_Info(void)
{
    uint8_t opr_buf[10] = {0};
    opr_buf[0] = (uint8_t)((GTP_REG_CONFIG_DATA+1) >> 8);
    opr_buf[1] = (uint8_t)((GTP_REG_CONFIG_DATA+1) & 0xFF);
    if (GTP_I2C_Read(GTP_ADDRESS, opr_buf, 10) < 0) return FAIL;

    uint16_t abs_x_max = (opr_buf[3] << 8) + opr_buf[2];
    uint16_t abs_y_max = (opr_buf[5] << 8) + opr_buf[4];
    GTP_DEBUG("X_MAX = %d, Y_MAX = %d", abs_x_max, abs_y_max);
    return SUCCESS;
}

int32_t GTP_Read_Version(void)
{
    uint8_t buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};
    if (GTP_I2C_Read(GTP_ADDRESS, buf, sizeof(buf)) < 0)
        return FAIL;

    if (buf[4] == '7')
    {
        GTP_INFO("IC Version: %c%c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);
        if (buf[2] == '9' && buf[3] == '1' && buf[4] == '7' && buf[5] == 'S')
            touchIC = GT917S;
    }
    else
        GTP_INFO("Unknown IC Version: %c%c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);
    return SUCCESS;
}

static int8_t GTP_I2C_Test(void)
{
    uint8_t test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
    int8_t ret = -1;
    for (int i = 0; i < 5; i++)
    {
        ret = GTP_I2C_Read(GTP_ADDRESS, test, 3);
        if (ret > 0) return ret;
    }
    return ret;
}

int32_t GTP_Init_Panel(void)
{
    int32_t ret = -1;
    I2C_Touch_Init();  /* 初始化 GPIO 及 I2C */

    ret = GTP_I2C_Test();
    if (ret < 0) {
        GTP_ERROR("I2C communication ERROR!");
        return ret;
    }

    GTP_Read_Version();

#if UPDATE_CONFIG
    uint8_t *config = (uint8_t *)malloc(GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
    if (config == NULL) {
        GTP_ERROR("malloc fail!");
        return -1;
    }

    config[0] = GTP_REG_CONFIG_DATA >> 8;
    config[1] = GTP_REG_CONFIG_DATA & 0xff;

    const uint8_t *cfg_info = CTP_CFG_GT917S;
    uint8_t cfg_info_len = CFG_GROUP_LEN(CTP_CFG_GT917S);

    memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
    memcpy(&config[GTP_ADDR_LENGTH], cfg_info, cfg_info_len);

    /* 填入屏幕分辨率 */
    config[GTP_ADDR_LENGTH+1] = LCD_PIXEL_WIDTH & 0xFF;
    config[GTP_ADDR_LENGTH+2] = LCD_PIXEL_WIDTH >> 8;
    config[GTP_ADDR_LENGTH+3] = LCD_PIXEL_HEIGHT & 0xFF;
    config[GTP_ADDR_LENGTH+4] = LCD_PIXEL_HEIGHT >> 8;

    /* 不交换 X/Y */
    config[GTP_ADDR_LENGTH+6] &= ~(X2Y_LOC);

    /* 计算校验和 (适用于 GT917S) */
    uint16_t check_sum = 0;
    for (int i = GTP_ADDR_LENGTH; i < (cfg_info_len + GTP_ADDR_LENGTH - 3); i += 2)
        check_sum += (config[i] << 8) + config[i + 1];
    check_sum = 0 - check_sum;
    config[cfg_info_len + GTP_ADDR_LENGTH - 3] = (check_sum >> 8) & 0xFF;
    config[cfg_info_len + GTP_ADDR_LENGTH - 2] = check_sum & 0xFF;
    config[cfg_info_len + GTP_ADDR_LENGTH - 1] = 0x01;

    for (int retry = 0; retry < 5; retry++)
    {
        ret = GTP_I2C_Write(GTP_ADDRESS, config, cfg_info_len + GTP_ADDR_LENGTH + 2);
        if (ret > 0) break;
    }

    HAL_Delay(100);  /* 等待芯片更新 */

    /* 回读验证 */
    {
        uint8_t buf[300];
        buf[0] = config[0];
        buf[1] = config[1];
        GTP_I2C_Read(GTP_ADDRESS, buf, sizeof(buf));
        for (int i = 3; i < cfg_info_len + GTP_ADDR_LENGTH - 3; i++)
        {
            if (config[i] != buf[i])
            {
                GTP_ERROR("Config fail at offset %d!", i);
                break;
            }
        }
    }

    free(config);
#endif

    GTP_IRQ_Enable();
    GTP_Get_Info();
    return 0;
}

int8_t GTP_Send_Command(uint8_t command)
{
    uint8_t command_buf[3] = {(uint8_t)(GTP_REG_COMMAND >> 8), (uint8_t)GTP_REG_COMMAND & 0xFF, command};
    for (int i = 0; i < 5; i++)
    {
        if (GTP_I2C_Write(GTP_ADDRESS, command_buf, 3) > 0)
            return 0;
    }
    return -1;
}

int GTP_Execu(int *x, int *y)
{
    uint8_t end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    uint8_t point_data[2 + 1 + 8 * 1 + 1] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    if (GTP_I2C_Read(GTP_ADDRESS, point_data, 12) < 0) return 0;

    uint8_t finger = point_data[GTP_ADDR_LENGTH];
    if (finger == 0x00) return 0;
    if ((finger & 0x80) == 0) goto exit_work_func;

    if (finger & 0x0f)
    {
        int input_x = point_data[3+1] | (point_data[3+2] << 8);
        int input_y = point_data[3+3] | (point_data[3+4] << 8);
        if (input_x < GTP_MAX_WIDTH && input_y < GTP_MAX_HEIGHT)
        {
            *x = input_x;
            //*y = 272 - input_y;   /* 根据屏幕高度翻转 Y 轴 */
           
            *y = input_y;        /* 不翻转 Y 轴 */
            
            GTP_I2C_Write(GTP_ADDRESS, end_cmd, 3);
            return finger & 0x0f;
        }
    }

exit_work_func:
    GTP_I2C_Write(GTP_ADDRESS, end_cmd, 3);
    return 0;
}

/* ---------------- 中断服务函数（如需在外部使用，请勿重复定义） ---------------- */
/* H743 的 EXTI15_10 中断已在 stm32h7xx_it.c 中，这里提供一个调用示例 */
/*
void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GTP_INT_GPIO_PIN) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GTP_INT_GPIO_PIN);
        GTP_TouchProcess();
    }
}
*/