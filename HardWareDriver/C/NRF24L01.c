/*    
      ____                      _____                  +---+
     / ___\                     / __ \                 | R |
    / /                        / /_/ /                 +---+
   / /   ________  ____  ___  / ____/___  ____  __   __
  / /  / ___/ __ `/_  / / _ \/ /   / __ \/ _  \/ /  / /
 / /__/ /  / /_/ / / /_/  __/ /   / /_/ / / / / /__/ /
 \___/_/   \__,_/ /___/\___/_/    \___ /_/ /_/____  /
                                                 / /
                                            ____/ /
                                           /_____/
NRF24L01.c file
编写者：小马  (Camel)
作者E-mail：375836945@qq.com
编译环境：MDK-Lite  Version: 4.23
初版时间: 2014-01-28
功能：
1.NRF24L01初始化
2.接口采用stm32的硬件SPI_1接口
------------------------------------
*/
#include "NRF24L01.h"
#include "spi.h"
#include "UART1.h"
#include "stdio.h"
#include "stdlib.h"
#include "ConfigTable.h"

u8 TxBuf[32];//发送数组
u8 RxBuf[32];//接收数组



//修改该接收和发送地址，可以供多个飞行器在同一区域飞行，数据不受干扰
u8  TX_ADDRESS[TX_ADR_WIDTH]= {0x34,0xc3,0x10,0x10,0x00};	//本地地址
u8  RX_ADDRESS[RX_ADR_WIDTH]= {0x34,0xc3,0x10,0x10,0x11};	//接收地址	

uint8_t NRF24L01_RXDATA[RX_PLOAD_WIDTH];//nrf24l01接收到的数据
uint8_t NRF24L01_TXDATA[RX_PLOAD_WIDTH];//nrf24l01需要发送的数据


//写寄存器
uint8_t NRF_Write_Reg(uint8_t reg, uint8_t value)
{
    uint8_t status;
    SPI_CSN_L();
    status = SPI_RW(reg);  
    SPI_RW(value);		  /* 写数据 */
    SPI_CSN_H();					  /* 禁止该器件 */
    return 	status;
}


//读寄存器
uint8_t NRF_Read_Reg(uint8_t reg)
{
    uint8_t reg_val;
    SPI_CSN_L();					 
    SPI_RW(reg);			  
    reg_val = SPI_RW(0);	  /* 读取该寄存器返回数据 */
    SPI_CSN_H();	
 
    return 	reg_val;
}


//写缓冲区
uint8_t NRF_Write_Buf(uint8_t reg, uint8_t *pBuf, uint8_t uchars)
{
    uint8_t i;
    uint8_t status;
    SPI_CSN_L();				        /* 选通器件 */
    status = SPI_RW(reg);	/* 写寄存器地址 */
    for(i=0; i<uchars; i++)
    {
        SPI_RW(pBuf[i]);		/* 写数据 */
    }
    SPI_CSN_H();						/* 禁止该器件 */
    return 	status;	
}


//读缓冲区
uint8_t NRF_Read_Buf(uint8_t reg, uint8_t *pBuf, uint8_t uchars)
{
    uint8_t i;
    uint8_t status;
    SPI_CSN_L();						/* 选通器件 */
    status = SPI_RW(reg);	/* 写寄存器地址 */
    for(i=0; i<uchars; i++)
    {
        pBuf[i] = SPI_RW(0); /* 读取返回数据 */ 	
    }
    SPI_CSN_H();						/* 禁止该器件 */
    return 	status;
}


//写数据包
void NRF_TxPacket(uint8_t * tx_buf, uint8_t len)
{	
    SPI_CE_L();		 //StandBy I模式	
    NRF_Write_Buf(WR_TX_PLOAD, tx_buf, len); 			 // 装载数据	
    SPI_CE_H();		 //置高CE，激发数据发送
}

//初始化
char NRF24L01_INIT(void)
{
   SPI1_INIT();
   return NRF24L01_Check();
}


//接收模式
void SetRX_Mode(void)
{
    SPI_CE_L();
	  NRF_Write_Reg(FLUSH_RX,0xff);//清除TX FIFO寄存器			 
  	NRF_Write_Buf(NRF_WRITE_REG+RX_ADDR_P0,(uint8_t*)RX_ADDRESS,RX_ADR_WIDTH);//写RX节点地址
   	NRF_Write_Reg(NRF_WRITE_REG+EN_AA,0x01);    //使能通道0的自动应答    
  	NRF_Write_Reg(NRF_WRITE_REG+EN_RXADDR,0x01);//使能通道0的接收地址  	 
  	NRF_Write_Reg(NRF_WRITE_REG+RF_CH,40);	     //设置RF通信频率		  
  	NRF_Write_Reg(NRF_WRITE_REG+RX_PW_P0,RX_PLOAD_WIDTH);//选择通道0的有效数据宽度 	    
  	NRF_Write_Reg(NRF_WRITE_REG+RF_SETUP,0x0f);//设置TX发射参数,0db增益,2Mbps,低噪声增益开启   
  	NRF_Write_Reg(NRF_WRITE_REG+CONFIG, 0x0f);//配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式 
    SPI_CE_H();
    printf("NRF24L01 set to RX mode...\r\n");
} 

//发送模式
void SetTX_Mode(void)
{
    SPI_CE_L();
    NRF_Write_Reg(FLUSH_TX,0xff);										//清除TX FIFO寄存器		  
    NRF_Write_Buf(NRF_WRITE_REG+TX_ADDR,(u8*)TX_ADDRESS,TX_ADR_WIDTH);		//写TX节点地址 
  	NRF_Write_Buf(NRF_WRITE_REG+RX_ADDR_P0,(u8*)RX_ADDRESS,RX_ADR_WIDTH); 	//设置TX节点地址,主要为了使能ACK	  
  	NRF_Write_Reg(NRF_WRITE_REG+EN_AA,0x01);     //使能通道0的自动应答    
  	NRF_Write_Reg(NRF_WRITE_REG+EN_RXADDR,0x01); //使能通道0的接收地址  
  	NRF_Write_Reg(NRF_WRITE_REG+SETUP_RETR,0x1a);//设置自动重发间隔时间:500us + 86us;最大自动重发次数:10次
  	NRF_Write_Reg(NRF_WRITE_REG+RF_CH,40);       //设置RF通道为40
  	NRF_Write_Reg(NRF_WRITE_REG+RF_SETUP,0x0f);  //设置TX发射参数,0db增益,2Mbps,低噪声增益开启   
  	NRF_Write_Reg(NRF_WRITE_REG+CONFIG,0x0e);    //配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式,开启所有中断
    SPI_CE_H();
  
  
} 

//启动NRF24L01发送一次数据
//txbuf:待发送数据首地址
//返回值:发送完成状况
u8 NRF24L01_TxPacket(u8 *txbuf)
{
	u8 sta;
 	//SPI1_SetSpeed(SPI_SPEED_8);//spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）   
	SPI_CE_L();//NRF24L01_CE=0;
  NRF_Write_Buf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);//写数据到TX BUF  32个字节
 	SPI_CE_H();//NRF24L01_CE=1;//启动发送	
	while(NRF24L01_IRQ!=0);//等待发送完成
	sta=NRF_Read_Reg(NRFRegSTATUS);  //读取状态寄存器的值	   
	NRF_Write_Reg(NRF_WRITE_REG+NRFRegSTATUS,sta); //清除TX_DS或MAX_RT中断标志
	if(sta&MAX_TX)//达到最大重发次数
	{
		NRF_Write_Reg(FLUSH_TX,0xff);//清除TX FIFO寄存器
		return MAX_TX; 
	}
	if(sta&TX_OK)//发送完成
	{
		return TX_OK;
	}
	return 0xff;//其他原因发送失败
}



//接收函数
u8 NRF24L01_RxPacket(u8 *rxbuf)
{
	u8 sta;		    							   
  //SPI2_SetSpeed(SPI_SPEED_4); //spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）   
	sta=NRF_Read_Reg(NRFRegSTATUS);  //读取状态寄存器的值    	 
	NRF_Write_Reg(NRF_WRITE_REG+NRFRegSTATUS,sta); //清除TX_DS或MAX_RT中断标志
	if(sta&RX_OK)//接收到数据
	{
		NRF_Write_Buf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//读取数据
		NRF_Write_Reg(FLUSH_RX,0xff);//清除RX FIFO寄存器 
		return 0; 
	}	   
	return 1;//没收到任何数据
}		

//给遥控器NRF24L01设置随机的地址
void NRF24L01_SetTxAddr(void)
{
	unsigned time = micros();
	
	printf("time is 0x%x\r\n",time);
	TX_ADDRESS[4] = (u8)time;
	
	
	//保存到EEPROM中
	SaveParamsToEEPROM();
}


//判断SPI接口是否接入NRF芯片是否可用
u8 NRF24L01_Check(void) 
{ 
   u8 buf[5]={0xC2,0xC2,0xC2,0xC2,0xC2}; 
   u8 buf1[5]; 
   u8 i=0; 
	 
	 //Set a rand TX address in init
	 if(0x00 == TX_ADDRESS[4]){
		 NRF24L01_SetTxAddr();
	 }
	 
   /*写入5 个字节的地址.  */ 
   NRF_Write_Buf(NRF_WRITE_REG+TX_ADDR,buf,5); 
     
   /*读出写入的地址 */ 
   NRF_Read_Buf(TX_ADDR,buf1,5); 
   
    /*比较*/ 
   for (i=0;i<5;i++) 
   { 
      if (buf1[i]!=0xC2) 
      break; 
   } 
  
   if (i==5)   {printf("NRF24L01 found...\r\n");return 1 ;}        //MCU 与NRF 成功连接 
   else        {printf("NRF24L01 not found...\r\n");return 0 ;}        //MCU与NRF不正常连接    
} 


