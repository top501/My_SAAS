#include "file.h"
#include "ff.h"
#include "rtc.h"

#define LINE_WIDTH 29
u8 file_Lseek_flag =0;


filesystem File_System = { {0},0};		//实体一个文件系统数据结构

//实体化一个活动文件结构体
Active_File struct_active_file = {0,{0},{0}};
FILINFO fno_dir[ DAY_SUM] = {0};   //文件变量 FIL INFO
Dir_t DirName;                            //要打开的目录指针
/**文件读写数据存储单元**/
UINT br,bw;
char buf_new[37] = {0};
char buf_old[37] = {0};
FSIZE_t EverSeek_Size = 0;

/*时间推算程序*/
/*
 *创建连续7天的活动数据文件夹
 */
static void Create_SevenDay_file(volatile struct rtc_time* time,module_state *state,Active_File *file)
{
    u8 i = 0;
    u8 result = 0;    																									//文件操作结果
    u32 sec = 0;         																								//临时计算秒
    u8 Hour_num = 0;     																								//24小时临时变量
    u32 num_count = 0;   																								//一个小时3600个数据循环写入。
    static u32 count = 1;    																						//excel表序列号
    u16 temflseek,nowflseek;

    //##############################################################################################################################################################
    for(i = 0; i< 3; i++)
    {
        memset(buf_old,0,sizeof(buf_old));															//清理空间
        memset(buf_new,0,sizeof(buf_new));															//清理空间（这里好像多余了，下面有）

        /*文件名最长也只能是8个字节*/
        sprintf(buf_old,"0:Active_Data/%04d-%02d-%02d",time->tm_year,time->tm_mon,time->tm_mday+i);		//将时间（年月日）转换到字符串中

        result = f_mkdir(buf_old);																			//Create a sub-directory 根据 上面由时间转换得到的字符串，创建子文件夹	//0:Active_Data/2018-04-23

        if(result)																											//如果创建文件夹失败
        {
            state->FATFS_state = ERR;
        }

        //============================================================================================================================================================

        for(Hour_num = 0; Hour_num < 24; Hour_num++) 										//0~23点，循环，生成csv文件
        {
            printf("第 %d 张表开始\r\n",Hour_num);
            memset(buf_new,0,sizeof(buf_new));													//清理空间

            sprintf(buf_new,"/%02d.csv",Hour_num);											//将时间（小时）转换到字符串中
            strcat(buf_old,buf_new);																		//将文件名 添加在 路径名后面

            f_open(&file->file,buf_old, FA_CREATE_ALWAYS | FA_READ| FA_WRITE | FA_OPEN_APPEND); 			//打开创建文件
            //FA_CREATE_ALWAYS 创建一个新文件。如果文件已存在，则它将被截断并覆盖。
            //&file->file 存放文件操作符

            memset(buf_old,0,sizeof(buf_old));																												//清理空间

            sprintf(buf_old,"序号, 活动量,光强 ,温度 ,声音强度\r");																			//文件中，首行写入的文字，作为栏目名称
            temflseek =(u16) f_tell(&file->file);
            f_write(&file->file,(const void*)buf_old,strlen(buf_old),&bw);
            //写入头数据
            printf("--->line size =%d\n",sizeof(volatile unsigned char)+sizeof(short int)+sizeof(float)+sizeof(unsigned char)+sizeof(unsigned char));

            //-----------------------------------------------------------------------------------------------------------------------------------------------------------
            for(num_count = 0; num_count < 3600; num_count++)																					//循环写入文件初始化数据	（）
            {

                memset(buf_old,0,sizeof(buf_old));
                sprintf(buf_old,"%04d, %04d, %.3f, %04d, %04d\r",count,-1,0.0,0,0);											//count行计数
                count++;
                f_write(&file->file,(const void*)buf_old,strlen(buf_old),&bw);												//写入表数据
                printf("%d\r",count+1);
            }

            f_close(&file->file);																																			//关闭文件
            memset(buf_old,0,sizeof(buf_old));
            count = 1;    //第二张表系列号开始

            sprintf(buf_old,"0:Active_Data/%04d-%02d-%02d",time->tm_year,time->tm_mon,time->tm_mday+i);
        }
    }
}



/*
	创建SD需要的所有文件目录
*/

static __INLINE void creat_directory(module_state *state)
{

    f_mkdir("0:Active_Data");         																																//建立声音文件夹
    f_mkdir("0:Error_Information");    																																//建立存储历史错误信息
    //Create_SevenDay_file(&systmtime,&module,&struct_active_file);																			//创建几天的文件，并初始化其中的数据
}

/**
**找到最旧文件
*/
static FRESULT Print_Directory(volatile struct rtc_time* time,Active_File *file)
{
    FRESULT res;
    FILINFO fileinfo; 																	//用于排序，临时储存数据    文件信息
    u8 count = 0;  																			//文件数组计数
    u8 i = 0;
    u8 j = 0;

    printf("##  正在进行 子文件夹时间排序 操作  ================\r\n");
    if(DirName.Dir_restate == 0)
    {
        printf("-----------------------------> 【信息】  文件夹	0:/Active_Data 没有打开，需要被打开 \n");
        res = f_opendir(&DirName.DIR,"0:/Active_Data");			//打开文件夹0:/Active_Data
        if( res != FR_OK)																		//如果打开失败
        {
            printf("-----------------------------> 【失败】  打开文件夹			0:/Active_Data \n");
            return FR_DISK_ERR;
        }
        else 																								//成功打开 0:/Active_Data  ，目录指针保存在 DirName.DIR
        {
            printf("-----------------------------> 【成功】  打开文件夹			0:/Active_Data \n");
            DirName.Dir_restate = ~0;      									//目录指针 有效
        }
    }
    else
    {
        printf("-----------------------------> 【信息】  文件夹	0:/Active_Data 已经被打开，不需要再次打开 \n");
    }

    printf("-----------------------------> 【开始】	循环读出所有此文件夹\n");
    for(;;)																							//！！！死循环，直到  找完所有的子文件
    {
        res = f_readdir(&DirName.DIR,&fileinfo);			 	//读取 0:/Active_Data 目录指针 DirName.DIR 下  的 文件信息 ，并 存储到 fileinfo临时变量中
        //f_readdir    Read an directory item						//在上一次的基础上，依次向后读取

        if((res != FR_OK)  ||(fileinfo.fname[0] == 0 )) //如果没有读成功，或者说读取到的信息中 主文件名为空  //意思就是  在0:/Active_Data  下读完所有的子文件夹信息之后，直接跳出
        {
            printf("-----------------------------> 【结束】	循环结束\n");
            break;																			//！！！查找子文件夹  结束
        }

        if(fileinfo.fname[0] == '2')   									//如果读取到文件夹，且文件夹名 以 2为开头  不是200,201,202,,,,211，这里是 2018 的 ‘2’
        {
            //将该文件夹信息 存到 文件夹信息数组里面。
            fno_dir[count] = fileinfo;

            printf("----------------------------->	【信息】		子文件夹： %s 日期 ：%d 时间：%d\r\n",fno_dir[count].fname,fno_dir[count].fdate,fno_dir[count].ftime);
            count++;

        }
    }
    printf("-----------------------------> 【信息】	子文件夹个数 ：count=%d\n",count);

    //冒泡排序法 对count个文件夹进行排序
    printf("-----------------------------> 【开始】	开始排序\n");
			
    for(i = 0; i < count-1; i ++)
    {   //时间排序找出最早建立的一个文件夹
        for(j= 0; j< count-1-i; j++)
        {
						#if 0
            if( ((unsigned int)fno_dir[j].fdate + (unsigned int)fno_dir[j].ftime)	< ((unsigned int)fno_dir[j+1].fdate + (unsigned int)fno_dir[j+1].ftime) )		//是否使用错误
            {
                fileinfo = fno_dir[j];
                fno_dir[j] = fno_dir[j+1];
                fno_dir[j+1] = fileinfo;
            }
						#else
					 if( strcmp(fno_dir[j].fname,fno_dir[j+1].fname)>0)		//20180515   > 20180513
            {
                fileinfo = fno_dir[j];
                fno_dir[j] = fno_dir[j+1];
                fno_dir[j+1] = fileinfo;
            }
						
						#endif
        }
    }
		
		
    printf("-----------------------------> 【结束】	结束排序\n");
////    //调试专用================================================================================================
//    for( j = 0; j < count; j++)
//    {
//        printf("当前文件夹  按照文件名称排序 %d ==> %s    时间 = %d \n ",j+1,fno_dir[j].fname,(unsigned int)fno_dir[j].fdate + (unsigned int)fno_dir[j].ftime);
//    }


    f_closedir(&DirName.DIR);
    DirName.Dir_restate = 0;
    printf("-----------------------------> 【信息】	关闭文件夹 0:/Active_Data \n");
    printf("~~~~结束 子文件夹时间排序 操作 ~~~~ \n");
    return res;
}


/**
**更改目录文件名字
*/

static FRESULT Change_Directory(volatile struct rtc_time* time,Active_File *file) {
    FRESULT res;
    memset(buf_new,0,sizeof(buf_new));
    memset(buf_old,0,sizeof(buf_old));

    //2018-04-23
    printf("##  正在进行 更改目录 操作  =============\r\n");
//文件夹重命名
    sprintf(buf_old,"0:/Active_Data/%s",fno_dir[0].fname);																																					//使用时间最前的那个文件夹 2018-04-16
    printf("-----------------------------> 【信息】	得到时间最靠前的文件夹名称：%s\n",buf_old);

    printf("-----------------------------> 【信息】	当前时间 = %d:%d:%d:%d:%d:%d\r\n",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour,time->tm_min,systmtime.tm_sec);
    sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d",time->tm_year,time->tm_mon,time->tm_mday);
    printf("-----------------------------> 【信息】	生成时间目标的文件夹名称：%s\n",buf_new);

    res = f_rename( buf_old,buf_new);																																																//修改文件夹名称，用2018-04-23，替换2018-04-16
    if(res != FR_OK)
    {
        printf("-----------------------------> 【失败】	文件夹重命名 \n");
        return res;
    }
    printf("-----------------------------> 【成功】	文件夹重命名\n");

//文件夹修改时间
    fno_dir[0].fdate = (WORD) (((time->tm_year-1980) *512U ) | time->tm_mon *32U | time->tm_mday);																	//修改变量日期（还没有直接修改文件夹）
    fno_dir[0].ftime = (WORD) (time->tm_hour *2048U | time->tm_min *32U | time->tm_sec / 2U);																				//修改变量时间（还没有直接修改文件夹）
    printf("-----------------------------> 【信息】	存储时间到数组中\n");

    res = f_chmod(buf_new,AM_DIR,AM_ARC);																																														//修改文件或子目录的属性  Change attribute of a file or sub-directory
    /*
    属性			描述
    Attribute Description
    AM_RDO 		Read only		只读属性
    AM_ARC 		Archive			档案属性
    AM_SYS 		System			系统属性
    AM_HID 		Hidden			隐藏属性
    */
    if(res != FR_OK)
    {
        printf("-----------------------------> 【失败】	修改文件夹属性\n");
        return res;
    }
    printf("-----------------------------> 【成功】	修改文件夹属性\n");


    res = f_utime((const char *)buf_new,(const FILINFO*) &fno_dir[0]);																															//修改文件夹时间戳  Change timestamp of a file or sub-directory
    if(res == FR_OK)
    {
        //printf("更改时间成功f_utime res : %d\r\n",res);
        printf("-----------------------------> 【失败】	修改文件夹时间\n");

    }
    else
    {
        //printf("更改时间失败f_utime res : %d\r\n",res);
        printf("-----------------------------> 【失败】	修改文件夹时间\n");
    }


    //sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour);


    if( time->tm_hour < 12)
    {
        sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+200);
    }
    else
    {
        sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+100);
    }
    printf("-----------------------------> 【信息】	得到目标csv文件名称：%s\n",buf_new);
    printf("-----------------------------> 【信息】	尝试打开文件：%s .csv\n",buf_new);
    res = f_open(&file->file,buf_new, FA_OPEN_ALWAYS | FA_WRITE | FA_READ );																												//根据秒，找到文件中的第多少行

    if(!res) 																																																												//csv文件打开成功
    {
        printf("-----------------------------> 【成功】	打开文件\n");
        f_lseek(&file->file,LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5); //( (time->tm_min*60 )+time->tm_sec))								//1~~~3600行，行定位
        printf("-----------------------------> 【信息】	f_lseek 文件行定位%d\n",LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5);
        file->res_state = ~0;  //文件打开标志
    }
    else
    {
        printf("-----------------------------> 【失败】	打开文件\n");
        return res;
    }
    return res;
}


/**管理文件夹，如果没有该文件夹就将时间最旧的更改成当前时间
 *
*/
FRESULT Find_Creat_Directory(volatile struct rtc_time* time,Active_File *file)
{
    FRESULT res;
    u8 H_Time = 0;
    u32 i = 0;
    printf("# 管理文件夹 Find_Creat_Directory =====================================\n");
    if(!file->res_state)																																							//文件没有打开
    {
        //如果没有打开
        printf("-----------------------------> 【信息】	此时没有文件被打开\n");
        sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d",time->tm_year,time->tm_mon,time->tm_mday);		//目标文件夹名称 0:/Active_Data/2018-04-12

        res = f_opendir(&DirName.DIR,"0:/Active_Data");																								//打开文件夹		0:/Active_Data

        if( res != FR_OK)																																							//文件夹打开失败
        {
            printf("-----------------------------> 【失败】	打开文件夹	0:/Active_Data\n");
            return FR_DISK_ERR;
        }
        else 																																													//文件夹打开成功
        {
            printf("-----------------------------> 【成功】	打开文件夹	0:/Active_Data\n");
            DirName.Dir_restate = ~0;
        }

        res = f_stat(buf_new,fno_dir);																																//查看文件夹(0:/Active_Data/2018-04-12)的状态， Check existance of a file or sub-directory

        f_closedir(&DirName.DIR);																																			//关闭文件夹		0:/Active_Data
        DirName.Dir_restate = 0;
        printf("-----------------------------> 【信息】	关闭文件夹	0:/Active_Data\n");

        if(!res)																																											//如果当天的文件夹(0:/Active_Data/2018-04-12)已存在
        {
            printf("-----------------------------> 【信息】	今天的文件夹已存在\n");
            if( time->tm_hour < 12)  																																	//根据小时，定位文件夹中的csv文件。如果是0到11点 ，则 以 2为开头，  200，201,202,203,,,,,211
            {
                printf("-----------------------------> 【信息】	此时是上午\n");
                sprintf(buf_new,"0:/Active_Data/%s/%02d.csv",fno_dir[0].fname,time->tm_hour+200);
            }
            else																																											//根据小时，定位文件夹中的csv文件。如果是12点到23点，则以1为开头，112，114，115，，，，123
            {
                printf("-----------------------------> 【信息】	此时是下午\n");
                sprintf(buf_new,"0:/Active_Data/%s/%02d.csv",fno_dir[0].fname,time->tm_hour+100);
            }


            res = f_open(&file->file,buf_new, FA_OPEN_ALWAYS | FA_WRITE | FA_READ );									//打开相应的csv文件（0:/Active_Data/2018-04-12/112.csv）,保持打开，且为读写模式

            if(!res)																																									//如果打开csv文件成功
            {
                printf("-----------------------------> 【成功】	打开csv文件\n");
                file->res_state = ~0;  																																//文件打开标志

                //定位到对应的行  比如 中午12:43：53   ：        2018-04-12 112.csv 中，43*60+ 53

                f_lseek(&file->file,LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5);//( (time->tm_min*60 )+time->tm_sec))
                printf("-----------------------------> 【信息】	f_lseek 行定位 %d \n",LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5);
            }

            else																																											//如果打开csv文件失败
            {
                printf("-----------------------------> 【失败】	打开csv文件\n");
                return res;
            }
        }
        else   																																												//如果文件夹(0:/Active_Data/2018-04-12)不存在
        {
            printf("-----------------------------> 【信息】	今天的文件夹不存在\n");
            Print_Directory(time,file);																																//找到全部的文件夹，并更具时间排序
            Change_Directory(time,file);																															//将最先前的文件夹 修改为此时所需要的那个文件夹，并重新定位到文件夹中
        }
    }
    else
    {
        printf("-----------------------------> 【信息】	此时已有文件被打开\n");
    }
    return res;
}


/*********************************************************************************
**函数功能： 小时时间到，更加当前小时打开相应小时文件夹
**
**time :当前时间结构体
**file：活动文件结构体
**返回值:
*********************************************************************************/
FRESULT Open_ActiveExcel(volatile struct rtc_time* time,Active_File *file)
{
    FRESULT res;
    if(!file->res_state) 																																																				//如果当前没有正在打开的文件
    {
        memset(buf_new,0,sizeof(buf_new));

        if( time->tm_hour < 12)
        {

            sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+200);

        }
        else
        {
            sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+100);

        }
        f_open(&file->file,buf_new,FA_OPEN_ALWAYS | FA_WRITE | FA_READ);

        if(time->tm_sec > 0)
        {
            time->tm_sec = 0;
        }

        f_lseek(&file->file,LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5); //( (time->tm_min*60 )+time->tm_sec))
        file->res_state = ~0;
    }
    else 																																																												//如果已经打开了某文件
    {
        f_close(&file->file);																																																		//关闭文件
        memset(buf_new,0,sizeof(buf_new));																																											//清空缓存


        if( time->tm_hour < 12)
        {
            sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+200);//得到目标文件名
        }
        else
        {
            sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour+100);//得到目标文件名
        }


        f_open(&file->file,buf_new,FA_OPEN_ALWAYS | FA_WRITE | FA_READ);																												//打开文件

        if(time->tm_sec > 0)
        {
            time->tm_sec = 0;
        }
        f_lseek(&file->file,LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5); //( (time->tm_min*60 )+time->tm_sec))						//定位到行
    }
    return res;
}

/*
 * 向文件系统，写入一组数据
*/
static FRESULT Write_Data(volatile struct rtc_time* time,Active_File *file,Environment_t *EV_data)
{
    FRESULT res;
		int row,action,light,temp,voice;
    if(file->res_state)
    {
        memset(buf_new,0,sizeof(buf_new));
				row=(time->tm_min*60 )+time->tm_sec+1;
				action=AD345_strcut_Data.sum_count;
				light=(int)EV_data->Light_data;
				temp=EV_data->Temperature_data;
				voice=EV_data->Voice_data;
			
				if(row		>=10000)row		=9999;
				if(action	>=10000)action=9999;
				if(light	>=10000)light	=9999;	//光线在阳光下，数值会达到16000+  ，这会导致文件错行
				if(temp		>=10000)temp	=9999;
				if(voice	>=10000)voice	=9999;
			
        //sprintf(buf_new,"%04d, %04d, %.3f, %04d, %04d\r",(time->tm_min*60 )+time->tm_sec+1,AD345_strcut_Data.sum_count,EV_data->Light_data,EV_data->Temperature_data,EV_data->Voice_data); //
        sprintf(buf_new,"%04d, %04d, %04d, %04d, %04d\r",row,action,light,temp,voice);
        //printf("%04d, %04d, %04d, %04d, %04d\r\n",(time->tm_min*60 )+time->tm_sec+1,AD345_strcut_Data.sum_count,EV_data->Light_data,EV_data->Temperature_data,EV_data->Voice_data); //
        //printf("%04d, %04d, %04d, %04d, %04d,%04d\r\n",time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour,time->tm_min,time->tm_sec);


        f_lseek(&file->file,LINE_WIDTH + (LINE_WIDTH * ( (time->tm_min*60 )+time->tm_sec))+5);

        res = f_write(&file->file,(const void*)buf_new,strlen(buf_new),&bw);//写入表数据
        if(!res) {
            // printf("写入数据成功\r\n");
            return FR_OK;
        } else printf("写入数据失败 ：%d\r\n",res);
    }
    else  return FR_NO_PATH;
}
/*-------------------------------------------------------------------------*/
/* Flash需要创建的所有文件函数                                         	   */
/*------------------------------------------------------------------------**/
void file_struct_init(module_state *state, filesystem* fs)
{
    FRESULT result; //文件函数操作结果
    result = f_mount(&fs->FS_Sysyem,"0:",0);
    if(result)
    {
        state->FATFS_state = ERR;
        printf("文件系统挂载不成功\r\n");
    }
    else if(result == FR_OK) {
        fs->FATFS_state = OK; //文件系统已经创建
        printf("文件系统挂载成功\r\n");
    }
    result = f_setlabel((const TCHAR*)"0:BB-CARE");  //更改u盘属性
    if(result != FR_OK) {
        state->FATFS_state = ERR;
        printf( "更改U盘属性不成功 = %d\r\n",result);
    }
    creat_directory(state);
    if(systmtime.tm_hour < 12) 
		{
        Find_Creat_Directory(&BeformSystmtime,&struct_active_file);   //找到最新文件 ，改名字，打开相应的EXCLE表
    }
		else 
		{
        Find_Creat_Directory(&systmtime,&struct_active_file);   //找到最新文件 ，改名字，打开相应的EXCLE表
    }

}




/*
	文件系统管理，管理文件系统卸载和挂载
	返回值：返回当前文件系统状态
*/
u8 FileSystem_ManegerProcess(filesystem *file_system,module_state* state)
{
    if(state->run) {
        if(!file_system->FATFS_state) {
            f_mount(&file_system->FS_Sysyem,"0:",1);
            file_system->FATFS_state = ~0;
        }
    } else {
        if(file_system->FATFS_state) {
            if( !struct_active_file.res_state ) {
                f_mount(NULL,"0:",1);            //卸载文件系统
//				printf("文件系统卸载\r\n");
                file_system->FATFS_state = 0;
            } else {
                f_close(&struct_active_file.file);
                f_mount(NULL,"0:",1);            //卸载文件系统
//				printf("文件系统卸载\r\n");
                file_system->FATFS_state = 0;
            }
        }
    }
    return file_system->FATFS_state;
}


/*
 * 所有文件管理函数，查找文件夹，修改文件名
	返回值：返回当前文件系统状态
*/

void File_Manage(volatile create_flag *flag)
{
   // if(file_Lseek_flag)printf("\n  刚刚唤醒 ==> 刷新时间: %d-%d-%d %d:%d:%d\n",systmtime.tm_year,systmtime.tm_mon,systmtime.tm_mday,systmtime.tm_hour,systmtime.tm_min,systmtime.tm_sec);

    if(flag->Open_ActDerec_flag || file_Lseek_flag==1 ) 																//到达 12点，需要换文件夹
    {
        Find_Creat_Directory(&systmtime,&struct_active_file);
        flag->Open_ActDerec_flag = 0;
//    printf("open_ActDerec_flag ！= 0\r\n");
    }


    if( flag->Open_ActiveExcel_flag  || file_Lseek_flag==1)  //打开excel表							//到达 整点，需要换文件
    {
        if(systmtime.tm_hour < 12)
        {
            Open_ActiveExcel(&BeformSystmtime,&struct_active_file);
        }
        else
        {
            Open_ActiveExcel(&systmtime,&struct_active_file);					//找到csv文件并定位行
        }
        flag->Open_ActiveExcel_flag = 0;
//    printf("open_ActDerec_flag ！= 0\r\n");
    }


    if( flag->Write_Data_flag) //写数据																								//到1s，需要写数据
    {
        Write_Data(&systmtime,&struct_active_file,&Environment_data);
        flag->Write_Data_flag = 0;
    }

    file_Lseek_flag=0;

}
/*

*/

void Write_Flag_for_Sleep(int flag,int position)
{
    FRESULT res;
    if(struct_active_file.res_state)
    {
        memset(buf_new,0,sizeof(buf_new));
        sprintf(buf_new,"%04d, %04d, %04d, %04d, %04d\r",(systmtime.tm_min*60 )+systmtime.tm_sec+1,flag,flag,flag,flag);
		
        //if(file_Lseek_flag==0) //进入睡眠前，需要写入标志。唤醒后 行定位 在外面执行，不再此处执行（因为不准）
        f_lseek(&struct_active_file.file,LINE_WIDTH + (LINE_WIDTH * ( (systmtime.tm_min*60 )+systmtime.tm_sec+position))+5);

        res = f_write(&struct_active_file.file,(const void*)buf_new,strlen(buf_new),&bw);//写入表数据
        if(!res)
        {
            //printf("写入数据成功\r\n");
            printf("-----------------------------> 【成功】 睡眠标志 or 唤醒标志  写入 ：%d\r\n",res);
            //return FR_OK;
        } else printf("-----------------------------> 【失败】 睡眠标志 or 唤醒标志  写入 ：%d\r\n",res);
    }
    else
    {
        printf("-----------------------------> 【信息】 睡眠标志 or 唤醒标志  写入，无文件被打开 \r\n");
        //return FR_NO_PATH;
    }

}


/*
	参数解释：
flag_When						标志位 数值，代表  睡眠前，唤醒后，U盘插入，U盘拔出,关机，开机
flag_update_time		1代表 ，需要更新rtc时间
flag_flseek					1代表，需要重新定位
position						在时间基础上是否，前移或者后移
*/

void Write_Flag_ALL_Situation(int flag_When, int flag_update_time,int flag_flseek,int position)
{
    FRESULT res;

    printf("-----------------------------> 【开始】 写标志工作		开始\r\n");
    //是否刷新rct时间
    if(flag_update_time)
    {
        refresh_system_time();
        printf("-----------------------------> 【信息】 已刷新系统时间\r\n");
    }

    if(struct_active_file.res_state)
    {
        memset(buf_new,0,sizeof(buf_new));
        //将标志数值存入缓冲区
        sprintf(buf_new,"%04d, %04d, %04d, %04d, %04d\r",(systmtime.tm_min*60 )+systmtime.tm_sec+1+position,flag_When,systmtime.tm_year,systmtime.tm_mon,systmtime.tm_mday);

        //是否重新行定位
        if(flag_flseek)
        {
            f_lseek(&struct_active_file.file,LINE_WIDTH + (LINE_WIDTH * ( (systmtime.tm_min*60 )+systmtime.tm_sec+position))+5);
            printf("-----------------------------> 【信息】 已进行行定位+偏移位 %d \r\n",position);
        }

        //写入数据
        res = f_write(&struct_active_file.file,(const void*)buf_new,strlen(buf_new),&bw);//写入表数据
        if(!res)
        {
            //printf("写入数据成功\r\n");
            printf("-----------------------------> 【成功】 标志写入 ：%d\r\n",flag_When);
            //return FR_OK;
        }
        else
        {
            printf("-----------------------------> 【失败】 标志写入 ：%d\r\n",flag_When);
        }
    }
    else
    {
        //准备检查文件夹是否存在
        memset(buf_new,0,sizeof(buf_new));
				//目标文件夹名称 0:/Active_Data/2018-04-12
				if(systmtime.tm_hour>=12)
						sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d",systmtime.tm_year,systmtime.tm_mon,systmtime.tm_mday);		
				else
						sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d",BeformSystmtime.tm_year,BeformSystmtime.tm_mon,BeformSystmtime.tm_mday);
				
        res = f_opendir(&DirName.DIR,"0:/Active_Data");																								//打开文件夹		0:/Active_Data

        if( res != FR_OK)																																							//文件夹打开失败
        {
            printf("-----------------------------> 【失败】	打开文件夹	0:/Active_Data\n");
            return;
        }
        else 																																													//文件夹打开成功
        {
            printf("-----------------------------> 【成功】	打开文件夹	0:/Active_Data\n");
            DirName.Dir_restate = ~0;
        }

        res = f_stat(buf_new,fno_dir);																																//查看文件夹(0:/Active_Data/2018-04-12)的状态， Check existance of a file or sub-directory
        printf("-----------------------------> 【信息】	查询文件夹是否存在？\n");

        f_closedir(&DirName.DIR);																																			//关闭文件夹		0:/Active_Data
        DirName.Dir_restate = 0;
        printf("-----------------------------> 【信息】	关闭文件夹	0:/Active_Data\n");
				
				
				
				
				
        if(!res)																																											//如果当天的文件夹(0:/Active_Data/2018-04-12)已存在
        {
						printf("-----------------------------> 【信息】	目标文件夹:%s  已存在\n",buf_new);
            //准备打开文件
            printf("-----------------------------> 【信息】 无文件被打开 \r\n");

            printf("-----------------------------> 【信息】 准备临时打开文件\r\n");
            memset(buf_new,0,sizeof(buf_new));

            if( systmtime.tm_hour < 12)
            {
                sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",BeformSystmtime.tm_year,BeformSystmtime.tm_mon,BeformSystmtime.tm_mday,BeformSystmtime.tm_hour+200);
            }
            else
            {
                sprintf(buf_new,"0:/Active_Data/%04d-%02d-%02d/%02d.csv",systmtime.tm_year,systmtime.tm_mon,systmtime.tm_mday,systmtime.tm_hour+100);
            }

            res=f_open(&struct_active_file.file,buf_new,FA_OPEN_ALWAYS | FA_WRITE | FA_READ);					//打开文件
						
						
						
            if(!res)
            {
                printf("-----------------------------> 【成功】 打开文件 ：%s\r\n",buf_new);
            }
            else
            {
                printf("-----------------------------> 【失败】 打开文件 ：%s\r\n",buf_new);
                return;
            }
            struct_active_file.res_state = ~0;
						
						
        }
        else   																																												//如果文件夹(0:/Active_Data/2018-04-12)不存在
        {
            printf("-----------------------------> 【信息】	目标文件夹:%s  不存在\n",buf_new);
					
						if(systmtime.tm_hour>=12)
						{
							Print_Directory(&systmtime,&struct_active_file);																																//找到全部的文件夹，并更具时间排序
							Change_Directory(&systmtime,&struct_active_file);																																//将最先前的文件夹 修改为此时所需要的那个文件夹，并重新定位到文件夹中
						}
						else
						{
							Print_Directory(&BeformSystmtime,&struct_active_file);																																//找到全部的文件夹，并更具时间排序
							Change_Directory(&BeformSystmtime,&struct_active_file);	
						}
        }
				
				
				 //是否重新行定位
        if(flag_flseek)
        {
            f_lseek(&struct_active_file.file,LINE_WIDTH + (LINE_WIDTH * ( (systmtime.tm_min*60 )+systmtime.tm_sec+position))+5);
            printf("-----------------------------> 【信息】 已进行行定位+偏移位 %d \r\n",position);
        }
				
        printf("-----------------------------> 【信息】	准备写入数据\n");
        memset(buf_new,0,sizeof(buf_new));
        //将标志数值存入缓冲区
        sprintf(buf_new,"%04d, %04d, %04d, %04d, %04d\r",(systmtime.tm_min*60 )+systmtime.tm_sec+1+position,flag_When,flag_When,flag_When,flag_When);


        //写入数据
        res = f_write(&struct_active_file.file,(const void*)buf_new,strlen(buf_new),&bw);//写入表数据
        if(!res)
        {
            //printf("写入数据成功\r\n");
            printf("-----------------------------> 【成功】 标志写入 ：%d\r\n",flag_When);
            //return FR_OK;
        }
        else
        {
            printf("-----------------------------> 【失败】 标志写入 ：%d\r\n",flag_When);
        }

//				//关闭文件（因为本身就是关闭状态）
//				res = f_close(&struct_active_file.file);
//				if(!res)
//        {
//            printf("-----------------------------> 【成功】 关闭文件 ：%d\r\n",res);
//        }
//        else
//        {
//            printf("-----------------------------> 【失败】 关闭文件 ：%d\r\n",res);
//        }
    }

    printf("-----------------------------> 【结束】 写标志工作		结束\r\n");

}

/************endif***************/
