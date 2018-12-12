
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

typedef struct{                         //环境温度数据结构
	volatile unsigned char Tempet_run_flag;
	volatile unsigned char Voice_run_flag;
	volatile unsigned char Light_run_flag;
  volatile unsigned char Power_check_flag;
	
	
	unsigned char Temperature_data;
	unsigned char Voice_data;
	float         Light_data;	
  unsigned char Power_data;
}Environment_t;

extern Environment_t Environment_data;               //环境温度数值

#endif
