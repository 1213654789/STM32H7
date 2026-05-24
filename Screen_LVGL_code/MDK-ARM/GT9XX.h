#ifndef _GOODIX_GTXX_H
#define _GOODIX_GTXX_H

#include "stm32h7xx_hal.h"
#include "usart.h"

#ifndef NULL
  #define NULL        0
#endif

/* 串口调试输出函数声明 */
void GTP_UART_Send(uint8_t *data, uint16_t len);

/* ---------- 屏幕分辨率 ---------- */
#define LCD_PIXEL_HEIGHT    272
#define LCD_PIXEL_WIDTH     480

#define UPDATE_CONFIG    1    // 1: 更新配置; 0: 不更新配置

/* 读数据标志 */
#define I2C_M_RD        0x0001

/* I2C 消息结构体 */
struct i2c_msg {
    uint8_t  addr;
    uint16_t flags;
    uint16_t len;
    uint8_t  *buf;
};

/* 触摸屏参数 */
typedef struct {
    uint16_t max_width;
    uint16_t max_height;
    uint16_t config_reg_addr;
} TOUCH_PARAM_TypeDef;

typedef enum {
    GT917S = 0,
} TOUCH_IC;

extern TOUCH_IC touchIC;
extern const TOUCH_PARAM_TypeDef touch_param[];

#define GTP_MAX_HEIGHT   touch_param[touchIC].max_height
#define GTP_MAX_WIDTH    touch_param[touchIC].max_width
#define GTP_INT_TRIGGER  0
#define GTP_MAX_TOUCH    5

#define GTP_DRIVER_VERSION          "V2.2<2014/01/14>"
#define GTP_I2C_NAME                "Goodix-TS"
#define GTP_POLL_TIME               10
#define GTP_ADDR_LENGTH             2
#define GTP_CONFIG_MIN_LENGTH       186
#define GTP_CONFIG_MAX_LENGTH       256
#define FAIL                        0
#define SUCCESS                     1
#define SWITCH_OFF                  0
#define SWITCH_ON                   1

/* 寄存器定义 */
#define GTP_REG_BAK_REF             0x99D0
#define GTP_REG_MAIN_CLK            0x8020
#define GTP_REG_CHIP_TYPE           0x8000
#define GTP_REG_HAVE_KEY            0x804E
#define GTP_REG_MATRIX_DRVNUM       0x8069
#define GTP_REG_MATRIX_SENNUM       0x806A
#define GTP_REG_COMMAND             0x8040

#define GTP_COMMAND_READSTATUS      0
#define GTP_COMMAND_DIFFERENCE      1
#define GTP_COMMAND_SOFTRESET       2
#define GTP_COMMAND_UPDATE          3
#define GTP_COMMAND_CALCULATE       4
#define GTP_COMMAND_TURNOFF         5

#define GTP_FL_FW_BURN              0x00
#define GTP_FL_ESD_RECOVERY         0x01
#define GTP_FL_READ_REPAIR          0x02

#define GTP_BAK_REF_SEND            0
#define GTP_BAK_REF_STORE           1
#define CFG_LOC_DRVA_NUM            29
#define CFG_LOC_DRVB_NUM            30
#define CFG_LOC_SENS_NUM            31

#define GTP_CHK_FW_MAX              40
#define GTP_CHK_FS_MNT_MAX          300
#define GTP_BAK_REF_PATH            "/data/gtp_ref.bin"
#define GTP_MAIN_CLK_PATH           "/data/gtp_clk.bin"
#define GTP_RQST_CONFIG             0x01
#define GTP_RQST_BAK_REF            0x02
#define GTP_RQST_RESET              0x03
#define GTP_RQST_MAIN_CLOCK         0x04
#define GTP_RQST_RESPONDED          0x00
#define GTP_RQST_IDLE               0xFF

#define GTP_READ_COOR_ADDR          0x814E
#define GTP_REG_SLEEP               0x8040
#define GTP_REG_SENSOR_ID           0x814A
#define GTP_REG_CONFIG_DATA         touch_param[touchIC].config_reg_addr
#define GTP_REG_VERSION             0x8140

#define RESOLUTION_LOC              3
#define TRIGGER_LOC                 8
#define X2Y_LOC                     (1<<3)

#define CFG_GROUP_LEN(p_cfg_grp)    (sizeof(p_cfg_grp) / sizeof(p_cfg_grp[0]))

/* ---------------- 调试开关（所有打印已移除） ---------------- */
#define GTP_DEBUG_ON             1
#define GTP_DEBUG_ARRAY_ON       1
#define GTP_DEBUG_FUNC_ON        0

/* 串口调试输出（通过LPUART1） */
void GTP_UART_Printf(const char *prefix, const char *fmt, ...);

/* 简化版宏 - 只输出固定前缀，参数通过函数处理 */
#define GTP_INFO(fmt,args...)  GTP_UART_Printf("[GTP] ", fmt, ##args)
#define GTP_ERROR(fmt,args...) GTP_UART_Printf("[GTP ERR] ", fmt, ##args)
#define GTP_DEBUG(fmt,args...) GTP_UART_Printf("[GTP DBG] ", fmt, ##args)

#define GTP_DEBUG_ARRAY(array, num)
#define GTP_DEBUG_FUNC()

#define GTP_SWAP(x, y)                 do{\
                                         typeof(x) z = x;\
                                         x = y;\
                                         y = z;\
                                       }while (0)

/* 函数声明 */
int8_t  GTP_Reset_Guitar(void);
int32_t GTP_Read_Version(void);
void    GTP_IRQ_Disable(void);
void    GTP_IRQ_Enable(void);
int32_t GTP_Init_Panel(void);
int8_t  GTP_Send_Command(uint8_t command);
int     GTP_Execu(int *x, int *y);
void    GTP_TouchProcess(void);

#endif /* _GOODIX_GTXX_H_ */