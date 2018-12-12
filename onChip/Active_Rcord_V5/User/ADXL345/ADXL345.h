#ifndef __ADXL345_H
#define __ADXL345_H

#include "machine_control.h"
#include "file.h"
#include "Environment.h"
#define ADXL345_ADDRESS 0x53<<1     //器件地址

typedef struct Axis_data{            //ADX345数据类型
	short int x_axis;
	short int y_axis;
	short int z_axis;
	short int sum_count;
	volatile unsigned char run_flag;          //ADX345数据采集标志
}ADXL345_Data_t;

//软件取活动数据时间间隔
#define   ADXL_SAMPLE_CYCLE  200   //没各200ms取一次数据
#define   GET_ADXLDATA_CYCLE 1000  //实际需要得到活动数据周期1000ms

//两次活动之间加速度差值

#define X_D_VALUE   3    //x轴差值 
#define Y_D_VALUE   3    //y轴差值
#define Z_D_VALUE   3    //z轴差值

#define ADXDATA_MAX 600  //运行出现的最大活动值


/*寄存器地址定义*/
#define ADXL_R_ID   		0x00         	//芯片ID寄存器


#define OFSX						0x1E
#define OFSY						0x1F
#define OFSZ						0x20


#define THRESH_ACT			0x24					//活动阈值
#define THRESH_INACT		0x25					//静止阈值
#define TIME_INACT			0x26					//静止时间
#define ACT_INACT_CTL		0x27					//轴使能控制活动和静止检测

#define BW_RATE 				0x2C					//电源和速率设置
#define POWER_CTL				0x2D				
#define INT_ENABLE			0x2E					//中断使能控制
#define INT_MAP					0x2F					//中断映射控制

#define INT_SOURCE			0x30

#define DATA_FORMAT 		0X31         	//设置寄存器值
#define X_DATA_ADDRESS  0X32          //X轴数值0
#define Y_DATA_ADRESS   0X34          //Y轴数值0
#define Z_DATA_ADRESS   0X36          //Z轴数值0



extern ADXL345_Data_t AD345_Data;
extern ADXL345_Data_t AD345_strcut_Data ;  //ADX345数据变量


unsigned char Check_ADXL345(void);
void ADXL345_init(void);
void ADXL345_init_sleep_wakeup();
void Collect_data(ADXL345_Data_t *Active_data,Environment_t * Envir_data ,module_state* state);

//void ActiveData_ToBuf(module_state*state,volatile create_flag *flag, volatile struct rtc_time *time, Active_File *file ,t_ADXL345_Data  *data_source,Environment*Envir_Data);
//void ActiveData_ToBuf(char *path,module_state*state,volatile struct rtc_time *time, Active_File *file ,ADXL345_Data_t  *data_source,Environment_t *Envir_Data);
#endif


