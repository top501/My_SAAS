#include "time.h"
#include "ADXL345.h"
#include "rtc.h"
#include "err_control.h"
#include "usb.h"
#include "usart1.h"
//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

    //定时器TIM3初始化
    TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM3,ENABLE);
}


void TIM3_IRQHandler(void)   //TIM3中断
{
	static u8 led_counter=0;
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
    {

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx更新中断标志
			
			
        if( run_time_data.Temp_RunTime )
        {
            run_time_data.Temp_RunTime--;
					
            if(run_time_data.Temp_RunTime == 0)
            {
                Environment_data.Tempet_run_flag = ~0;
                run_time_data.Temp_RunTime = GetTempet_RunTime;
            }
        }
        if( run_time_data.voice_RunTime )
        {
            run_time_data.voice_RunTime--;
            if(run_time_data.voice_RunTime == 0)
            {
                Environment_data.Voice_run_flag = ~0;
                run_time_data.voice_RunTime = GetVoice_RunTime;
            }
        }
		
        if(run_time_data.ADXL345_RunTime)
        {
            run_time_data.ADXL345_RunTime--;
            if(run_time_data.ADXL345_RunTime == 0)
            {
                AD345_strcut_Data.run_flag = ~0;
                run_time_data.ADXL345_RunTime = ADXL345_RunTime;
            }
        }
        if(run_time_data.Light_RunTime)
        {
            run_time_data.Light_RunTime--;
            if(run_time_data.Light_RunTime == 0)
            {
                Environment_data.Light_run_flag = ~0;
                run_time_data.Light_RunTime = GetLight_RunTime;
            }
        }

        if( run_time_data.Power_RunTime ) {
            run_time_data.Power_RunTime--;
            if( run_time_data.Power_RunTime == 0 ) {
                Environment_data.Power_check_flag = ~0;
                run_time_data.Power_RunTime = POWER_CHECK_RunTime;
            }
        }
        if(module.machine_state == OK && module.run) {
            if( GreenLed_BlinkTime ) {
                GreenLed_BlinkTime --;
                if(GreenLed_BlinkTime == 0) 
								{
										
											if(led_counter<=20)
											{
												led_counter++;
												GREEN_LED_TOGGLE;
											}
											if(led_counter==21)GREEN_LED_OFF;
									
                    GreenLed_BlinkTime = GREEN_BLINK_TIME;
                }
            }
        }
        if( Usb_State == Connect)//USB线没有插入)
        {
            GREEN_LED_OFF; //绿灯灭
            if( chongdian_flag == chong_state ) {
                if( RedLed_BlinkTime ) {
                    RedLed_BlinkTime --;
                    if(RedLed_BlinkTime == 0) {
                        RED_LED_TOGGLE;
                        RedLed_BlinkTime = RED_BLINK_TIME;
                    }
                }
            } else if( chongdian_flag == chong_finsh ) {
                GREEN_LED_ON; //绿灯亮
                RED_LED_OFF; //红灯亮
            }
        }
		
		if(  usart1.receive_count ){
			if( usart1.receive_count != usart1.pro_receiver_count ){
				usart1.pro_receiver_count = usart1.receive_count;
			}
			else{
				usart1.receive_over_time--;
				if( usart1.receive_over_time == 0 ){
					usart1.usart_rx_ok = ~0;
					usart1.pro_receiver_count = 0;
					usart1.receive_count = 0;
				}
			}
		}
    }
}

