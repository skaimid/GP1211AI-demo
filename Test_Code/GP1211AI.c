//////////////////////////////////////////////////////////////////////////////////////
//    o     o  o o o o  o o o       	HardWare: Ver3.1  BY DONGFNEG           	//
//    o     o  o        o     o     	SoftWare: Ver3.0  BY DONGFENG           	//
//    o     o  o        o     o 	   	VFD Module   : FUTABA GP1211AI 128*64   	//
//    o     o  o o o o  o     o     	CONTROL MCU  : STC8H8K         				//
//     o   o   o        o     o        	SYSTEM CLOCK : 44.2368MHz            		//
//       o     o        o o o          	Design  Date : 2024/04/30	         		//
//                                		E-mail: dongfeng.dream@126.com          	//
//////////////////////////////////////////////////////////////////////////////////////

#include <STC8H.H>
#include <stdio.h>
#include <string.h>
#include <intrins.h>

#include "ASC1224.h"
#include "ASC816.h"
#include "ASC57.h"
#include "amp_graph.h"

#define VFD_CLKG_STROBE() (VFD_CLKG=0,_nop_(),VFD_CLKG=1,_nop_())
#define VFD_LAT_STROBE()  (VFD_LAT =1,_nop_(),VFD_LAT =0,_nop_())

sbit VFD_BK   = P2^1;   //PWM6
sbit VFD_LAT  = P2^6;
sbit VFD_SIG  = P2^2;
sbit VFD_CLKG = P2^4;
sbit VFD_SIA  = P2^3;				//MOSI
sbit VFD_CLKA = P2^5;				//SCLK

sbit HV_EN    = P0^0;   //
sbit FL_EN    = P0^1;   //

sbit K_U = P3^4;	//key +
sbit K_D = P3^5;	//key -
sbit K_M = P3^3;    //key menu


unsigned char xdata  DP_RAM[8][128];    //显示缓冲
unsigned char xdata  DP_BUF[2064];      //SPI 点阵显示缓

unsigned char        VFD_GRID_SCAN;
unsigned char       *DP_BUF_POINT;

bit			         DP_BUF_UPD;	    //dot buffer flash necessary flag

unsigned char xdata Disp_Brt_Data;	    //pwm duty , used for display brightness set


//---------------------------------------------------------------------------------------//

void Delay_10mS()		//@44.2368MHz
{
	unsigned char data i, j, k;
	i = 3;
	j = 63;
	k = 124;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void Delay_100mS()		//@44.2368MHz
{
	unsigned char data i, j, k;
	i = 23;
	j = 113;
	k = 248;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void Delay_1S()		//@44.2368MHz
{
	unsigned char data i, j, k;
	_nop_();
	i = 225;
	j = 106;
	k = 203;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

//---------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------//



//---------------------------------------------------------------------------------------//

void Disp_Buf_Update(void)	      		//vfd扫描
{
	unsigned char i=0;
	unsigned char VFD_GRID;
    unsigned char VFD_GRID_TMP=0;											
    unsigned char Dp_Ram_Temp=0;
    unsigned char Dp_Ram_Base_Addr=0;
    unsigned char *Dp_Buf_Ptr;
    
    Dp_Buf_Ptr = DP_BUF;

    for(VFD_GRID=0; VFD_GRID<43; VFD_GRID++)
    {
		VFD_GRID_TMP=42-VFD_GRID;

    	Dp_Ram_Base_Addr=(VFD_GRID_TMP>>1)*6;
    	for(i=0;i<8;i++)
    	{
			if((bit)(VFD_GRID_TMP&0x01))    	//双数列
    		{
				Dp_Ram_Temp =((DP_RAM[i][Dp_Ram_Base_Addr+5]<<1)&0x02);
		 		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+4]<<3)&0x08);
         		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+3]<<5)&0x20);
         		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+5]<<6)&0x80);
	        	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;		
         		Dp_Ram_Temp =   DP_RAM[i][Dp_Ram_Base_Addr+4]     &0x02;
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<2)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<3)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<5)&0x80);
				*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>1)&0x02);
         		Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+5]     &0x08;
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<2)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<4)&0x80);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>3)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>1)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<1)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<2)&0x80);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>4)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>2)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>1)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<1)&0x80);    
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>5)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>4)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>2)&0x20);
         		Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+3]     &0x80;
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
        	}
	    	else       							//单数列
    		{
				Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr+0]   &0x01;
           	 	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<2&0x04);
           	 	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<4&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<5&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>1&0x01);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<1&0x04);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<2&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<4&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>2&0x01);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>1&0x04);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<1&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<3&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+0]>>4&0x01);
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>2&0x04);
            	Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+2]   &0x10;
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<1&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>5&0x01);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>3&0x04);  
	        	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>2&0x10);
            	Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+1]   &0x40;
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>6&0x01);
	        	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>5&0x04);
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>3&0x10);   
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>1&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;  			         
        	}	
    	}  	
    }
}


//Display a 5*7 dots ASCII charactor at position (VFD_X,VFD_Y)
void VFD_DISP_ASC57(unsigned char VFD_X,unsigned char VFD_Y,unsigned char DAT)
{
	unsigned char i;
	for(i=0;i<5;i++)
	{
		DP_RAM[VFD_X][VFD_Y*8+i] = ASC57[((DAT-0x20)*5)+i];
	}
}

//Display 5*7 dots ASCII charactor string started at position (VFD_X,VFD_Y)
void VFD_DISP_ASC57_STR(unsigned char VFD_X,unsigned char VFD_Y,unsigned char *STR)
{
	while(*STR)
	{
		VFD_DISP_ASC57(VFD_X,VFD_Y++,*STR++);
	}
}


//Display a 8*16 dots ASCII charactor at position (VFD_X,VFD_Y)
void VFD_DISP_ASC816(unsigned char VFD_X,unsigned char VFD_Y,unsigned char DAT)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		DP_RAM[VFD_X*2][VFD_Y*8+i] = ASC816[((DAT-0x20)<<4)+i];
		DP_RAM[VFD_X*2+1][VFD_Y*8+i] = ASC816[((DAT-0x20)<<4)+8+i];
	}
}

//Display 8*16 dots ASCII charactor string started at position (VFD_X,VFD_Y)
void VFD_DISP_ASC816_STR(unsigned char VFD_X,unsigned char VFD_Y,unsigned char *STR)
{
	while(*STR)
	{
		VFD_DISP_ASC816(VFD_X,VFD_Y++,*STR++);
	}
}

//Display a 12*24 dots ASCII charactor at position (VFD_X,VFD_Y) , location by dot
void VFD_DISP_ASC1224(unsigned char VFD_X,unsigned char VFD_Y,unsigned char DAT)
{
	unsigned char i;
	for(i=0;i<12;i++)
	{
		DP_RAM[VFD_X][VFD_Y+i] = ASC1224[((DAT-0x20)*36)+i];
		DP_RAM[VFD_X+1][VFD_Y+i] = ASC1224[((DAT-0x20)*36)+12+i];
		DP_RAM[VFD_X+2][VFD_Y+i] = ASC1224[((DAT-0x20)*36)+24+i];
	}
}

//Display 12*24 dots ASCII charactor string started at position (VFD_X,VFD_Y)
void VFD_DISP_ASC1224_STR(unsigned char VFD_X,unsigned char VFD_Y,unsigned char *STR)
{
	while(*STR)
	{
		VFD_DISP_ASC1224(VFD_X,VFD_Y,*STR++);
		VFD_Y=VFD_Y+12;
	}
}

//Write a 32*32 dots picture to display buffer
void VFD_DISP_PIC_3232(unsigned char LCD_X,unsigned char LCD_Y,unsigned char *dat)			
{
    unsigned char i = 0;
    unsigned char j = 0;
    for(i = 0;i <4 ;i++)
    {
        for(j = 0;j < 32;j++)
        {
            DP_RAM[LCD_X+i][LCD_Y+j] = *dat;
            dat++;
        }
    }
}

//Write a 16*16 dots picture to display buffer
void VFD_DISP_PIC_1616(unsigned char LCD_X,unsigned char LCD_Y,unsigned char *dat)			
{
    unsigned char i = 0;
    unsigned char j = 0;
    for(i = 0;i <2 ;i++)
    {
        for(j = 0;j < 16;j++)
        {
            DP_RAM[LCD_X+i][LCD_Y+j] = *dat;
            dat++;
        }
    }
}

//Write a 128*64 dots picture to display buffer
void VFD_DISP_PIC_12864(unsigned char *dat)			
{
    unsigned char i = 0;
    unsigned char j = 0;
    for(i = 0;i <8 ;i++)
    {
        for(j = 0;j < 128;j++)
        {
            DP_RAM[i][j] = *dat;
            dat++;
        }
    }
}

//Clear the display ram
void DP_RAM_CLR(void)				
{
    unsigned char i = 0;
    unsigned char j = 0;

    for(i = 0;i < 8;i++)
    {
        for(j = 0;j < 128;j++)
        {
            DP_RAM[i][j] = 0x00;
        }
    }
	DP_BUF_UPD = 1;
}

//---------------------------------------------------------------------------------------//



//Timer0 used for VFD refresh
void Timer_0_Svr(void) interrupt 1
{
	unsigned char cycle_cnt = 48;
	if(VFD_GRID_SCAN==0)
	{
		DP_BUF_POINT  = DP_BUF;
		VFD_GRID_SCAN = 43;
		
		VFD_SIG=1;
		VFD_CLKG_STROBE();
		VFD_CLKG_STROBE();
		VFD_SIG=0;
		VFD_CLKG_STROBE();
		VFD_CLKG_STROBE();
		VFD_CLKG_STROBE();
	}
   	
    while((cycle_cnt--)>0)
	{	
		SPDAT = *DP_BUF_POINT++;		//write data to send buffer
		while (!(SPSTAT&0x80)); 
        SPSTAT = 0xc0; 
	}
	VFD_CLKG_STROBE();		
   	PWMB_CCR6  = 0;	
	VFD_LAT_STROBE();
	PWMB_CCR6  = Disp_Brt_Data; //Disp_Brt_Data;VFD_BK=0;
	--VFD_GRID_SCAN;
}
//---------------------------------------------------------------------------------------//



//---------------------------------------------------------------------------------------//

void Gpio_Init(void)
{
    //0.4 0.7push pull , others standard
    P0M0 = 0x03; P0M1 = 0x00; 
    //1.0~1 standard , 1.2 push pull , 1.3~5 standard , 1.6~7push-pull 
    P1M0 = 0x00; P1M1 = 0x00; 
    //2.0~3 5~7 push-pull , 2.4 standard
    P2M0 = 0x00; P2M1 = 0x00; //P2PU = 0x00; P2SR = 0x00;
    //3.0~2 standard , 3.3~7 push-pull
    P3M0 = 0x00; P3M1 = 0x00;
    //4.1~4 push pull , others standard
    P4M0 = 0x00; P4M1 = 0x00; 
    //5.4 push pull , others standard
    P5M0 = 0x00; P5M1 = 0x00;
    //6.7~5 push pull , others standard
    P6M0 = 0x00; P6M1 = 0x00;
    //7.0~5 pull pull , others standard
    P7M0 = 0x00; P7M1 = 0x00;
}

void Port_Switch(void)
{
    //| P_SW1 | addr | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    //|	 		  | A2H  | S1_S  | - | - | SPI_S | 0 | - |
    //| set value    | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |
    //uart 1 using P3.0 and P3.1 , spi using P2.2 P2.3 P2.4 P2.5
    P_SW1 = 0x04;	

    //| P_SW2 | addr |   7   | 6 | 5 | 4 |    3   |   2  |   1  |   0  |
    //|       | BAH  | EAXFR | - | I2C_S | CMPO_S | S4_S | S3_S | S2_S |
    //| set value    |   1   | 0 | 0 | 0 |    0   |   1  |   1  |   1  |
    //uart 4 using P5.2 P5.3 , uart 3 using P5.0 P5.1 , uart 2 using P4.6 P4.7
    //P_SW2 = 0x87;

    //| PWMB_PS | addr | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    //|         | feb6 |  C8PS |  C7PS |  C6PS |  C5PS |
    //| set valude     | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 |
    //PWM5 switched to P7.4 , PWM6 switched to P7.5
    //PWMB_PS = 0x0f;
}

void Pwm_Init(void)
{
	P_SW2 |= 0x80;
    PWMB_CCER1 = 0x00;                          //写CCMRx前必须先清零CCERx关闭通道
    PWMB_CCMR1 = 0x60;                          //设置CC5为PWMA输出模式
	PWMB_CCMR2 = 0x70;							//设置CC6为PWMA输出模式 
    PWMB_CCER1 = 0x11;                          //使能CC6 CC5通道
    PWMB_CCR5  = Disp_Brt_Data;                 //设置占空比时间
	PWMB_CCR6  = Disp_Brt_Data;                 //设置占空比时间
    PWM2_PSCR  = 2; 
    PWMB_ARR = 200;                             //设置周期时间
    PWMB_ENO = 0x05;                            //使能PWM6 PWM5端口输出
    PWMB_BKR = 0x80;                            //使能主输出
    PWMB_CR1 = 0x01;                            //开始计时
}

void Spi_Init(void)
{
    // |SPCTL ||| SSIG | SPEN | DORD | MSTR | CPOL | CPHA | SPR[1:0] |
    // |      |||   1  |   1  |   1  |   1  |   0  |   0  |   11     |
    //SPR -- 00--/4, 01--/8 , 10--/16 , 11--/2
    SPCTL = 0xf0;
    // |SPSTAT||| SPIF | WCOL |  NC  |  NC  |  NC  |  NC  |  NC  |
    //clear flags
    SPSTAT = 0xc0;
}

void Timer_0_Init(void)		//188微秒@44.2368MHz
{
    AUXR &= 0x7F;			//定时器时钟12T模式
    TMOD &= 0xF0;			//设置定时器模式
    TL0   = 0x4B;			//设置定时初始值
    TH0   = 0xFD;			//设置定时初始值
    TF0   = 0;				//清除TF0标志
    TR0   = 0;				//定时器0开始计时
}

void Spi_Send_Byte(unsigned char dat)
{
	SPDAT = dat;            
    while (!(SPSTAT&0x80)); 
    SPSTAT = 0xc0;         	
}
										 
void main(void)
{
	Disp_Brt_Data = 1;
    FL_EN = 1;
    HV_EN = 0;
    
    Port_Switch();
	Gpio_Init();					//gpio initialization
    Timer_0_Init();					//timer0 initialization
	Spi_Init();						//spi initialization
	Pwm_Init();

	Delay_100mS();				   	//waiting for system stable when powering on
    ET0 = 1;
	EA = 1; 						//enable global interrupt

	
	DP_RAM_CLR();					//Clear display ram
//	VFD_DISP_ASC57_STR(0,0,"VFD12864 ASC5X7  ");

//	VFD_DISP_ASC816_STR(1,0,"VFD12864 ASC8X16");

//	Disp_Buf_Update();				//update

//	TR0=1;							//start timer 0 counting for diaplay

//	Delay_1S();Delay_1S();Delay_1S();Delay_1S();Delay_1S();

//	TR0 = 0;

	VFD_DISP_PIC_12864(logo_12864);	//display logo
	Disp_Buf_Update();

	TR0 = 1;
    
    FL_EN = 0;
    Delay_100mS();
    HV_EN = 1;

	while(1)
	{	
		if(!K_U)
		{
			Delay_10mS();
			if(!K_U)
			{
				Disp_Brt_Data+=10;
                if(Disp_Brt_Data>=100) Disp_Brt_Data = 100;
				PWMB_CCR6  = Disp_Brt_Data;

			}
			while(!K_U);
		}

		if(!K_D)
		{
			Delay_10mS();
			if(!K_D)
			{
				Disp_Brt_Data-=10;
				if(Disp_Brt_Data==0) Disp_Brt_Data = 10;
				PWMB_CCR6  = Disp_Brt_Data;
			}
			while(!K_D);
		}
		
		VFD_DISP_ASC57(0,0,Disp_Brt_Data/1000 + 0x30);
		VFD_DISP_ASC57(0,1,Disp_Brt_Data%1000/100 + 0x30);
		VFD_DISP_ASC57(0,2,Disp_Brt_Data%1000%100/10 + 0x30);
		VFD_DISP_ASC57(0,3,Disp_Brt_Data%1000%100%10 + 0x30);
		Disp_Buf_Update();				//update
        Delay_100mS();
        Disp_Brt_Data++;
        if(Disp_Brt_Data>200) Disp_Brt_Data = 1;

	}
}