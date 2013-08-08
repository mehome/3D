/******************** (C) COPYRIGHT 2012 ***************** **************************
 * �ļ���  ��main.c
 * ����    ������1(USART1)����Եĳ����ն���1sΪ�����ӡ��ǰADC1��ת����ѹֵ         
 * ʵ��ƽ̨��STM32������
 * ��汾  ��ST3.5.0
 *
**********************************************************************************/
#include "stm32f10x.h"
#include "usart1.h"
#include "adc.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_spi.h"
#include "I2C.h"

// I2C functions


// ADC1ת���ĵ�ѹֵͨ��MDA��ʽ����SRAM
extern __IO uint16_t ADC_ConvertedValue;

// �ֲ����������ڱ���ת�������ĵ�ѹֵ			 

float ADC_ConvertedValueLocal;        

// �����ʱ
void Delay(__IO uint32_t nCount)
{
  for(; nCount != 0; nCount--);
} 
#define MPU6050SlaveAddress 0xD0
#define MPU6050_REG_WHO_AM_I 0x75

void I2C_Configuration(void)
{

}

unsigned char I2c_Buf[10];

/*************************************************** 
**������:I2C_ReadTmp 
**����:��ȡ tmp101�� 2���ֽ��¶� 
***************************************************/ 

void I2C_ReadTmp(void) 
{
	/*��������Ƿ�æ ���ǿ�  SCL ��SDA�Ƿ�Ϊ ��  */ 
	//printf("1");
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)); 
   
	/*����1�ֽ�1 Ӧ��ģʽ*/ 
	I2C_AcknowledgeConfig(I2C2, ENABLE); 
 
	/* ������ʼλ  */ 
     I2C_GenerateSTART(I2C2, ENABLE); 

	//printf("2");
     while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); /*EV5,��ģʽ*/ 
 
     /*����������ַ(д)*/ 
     I2C_Send7bitAddress(I2C2, MPU6050SlaveAddress, I2C_Direction_Transmitter); 
	
	//printf("3");
     while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	
			/*����Pointer Register*/ 
     I2C_SendData(I2C2, MPU6050_REG_WHO_AM_I);
	//printf("4");
     while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); /*�����ѷ���*/
   
			/*��ʼλ*/ 
			I2C_GenerateSTART(I2C2, ENABLE); 
	//printf("5");
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 
 
			/*����������ַ(��)*/ 
			I2C_Send7bitAddress(I2C2, MPU6050SlaveAddress, I2C_Direction_Receiver); 
	//printf("6");
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); 

			/* ��Temperature Register*/ 
			//while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)); /* EV7 */
			//I2c_Buf[0]= I2C_ReceiveData(I2C2); 
 
 
    /*�� Ϊ�����յ����һ���ֽں����һ��NACK���壬�ڶ������ڶ��������ֽ�֮��(�ڵ����ڶ���RxNE�¼�֮��)�������ACK λ�� 
			�� Ϊ�˲���һ��ֹͣ/����ʼ��������������ڶ������ڶ��������ֽ�֮��(�ڵ����ڶ���RxNE �¼�֮��)���� STOP/START λ�� 
			�� ֻ����һ���ֽ�ʱ���պ���EV6 ֮��(EV6_1ʱ�����ADDR֮��)Ҫ�ر�Ӧ���ֹͣ�����Ĳ���λ��*/ 
     I2C_AcknowledgeConfig(I2C2, DISABLE); //���һλ��Ҫ�ر�Ӧ���
     I2C_GenerateSTOP(I2C2, ENABLE);   //����ֹͣλ
 
	//printf("7");
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)); /* EV7 */ 
				I2c_Buf[1]= I2C_ReceiveData(I2C2); 
            
		/*�ٴ�����Ӧ��ģʽ*/ 
	//printf("8,");
		I2C_AcknowledgeConfig(I2C2, ENABLE); 
} 

#define NRF_CSN_HIGH(x) GPIO_SetBits(GPIOA,GPIO_Pin_1)
#define NRF_CSN_LOW(x) GPIO_ResetBits(GPIOA,GPIO_Pin_1)
#define NRF_CE_LOW(x) GPIO_ResetBits(GPIOA,GPIO_Pin_2)
#define NRF_CE_HIGH(x) GPIO_SetBits(GPIOA,GPIO_Pin_2)
#define NRF_Read_IRQ(x) GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)

#define NOP             0xFF  //�ղ���,����������״̬�Ĵ���	 
#define TX_ADDR         0x10  //���͵�ַ(���ֽ���ǰ),ShockBurstTMģʽ��,RX_ADDR_P0��˵�ַ���
#define NRF_WRITE_REG   0x20  //д���üĴ���,��5λΪ�Ĵ�����ַ
#define RX_ADDR_P0      0x0A  //����ͨ��0���յ�ַ,��󳤶�5���ֽ�,���ֽ���ǰ

#define RD_RX_PLOAD     0x61  //��RX��Ч����,1~32�ֽ�
#define WR_TX_PLOAD     0xA0  //дTX��Ч����,1~32�ֽ�
#define FLUSH_TX        0xE1  //���TX FIFO�Ĵ���.����ģʽ����
#define FLUSH_RX        0xE2  //���RX FIFO�Ĵ���.����ģʽ����
#define REUSE_TX_PL     0xE3  //����ʹ����һ������,CEΪ��,���ݰ������Ϸ���.
#define NOP             0xFF  //�ղ���,����������״̬�Ĵ���	 
//SPI(NRF24L01)�Ĵ�����ַ
#define CONFIG          0x00  //���üĴ�����ַ;bit0:1����ģʽ,0����ģʽ;bit1:��ѡ��;bit2:CRCģʽ;bit3:CRCʹ��;
                              //bit4:�ж�MAX_RT(�ﵽ����ط������ж�)ʹ��;bit5:�ж�TX_DSʹ��;bit6:�ж�RX_DRʹ��
#define EN_AA           0x01  //ʹ���Զ�Ӧ����  bit0~5,��Ӧͨ��0~5
#define EN_RXADDR       0x02  //���յ�ַ����,bit0~5,��Ӧͨ��0~5
#define SETUP_AW        0x03  //���õ�ַ���(��������ͨ��):bit1,0:00,3�ֽ�;01,4�ֽ�;02,5�ֽ�;
#define SETUP_RETR      0x04  //�����Զ��ط�;bit3:0,�Զ��ط�������;bit7:4,�Զ��ط���ʱ 250*x+86us
#define RF_CH           0x05  //RFͨ��,bit6:0,����ͨ��Ƶ��;
#define RF_SETUP        0x06  //RF�Ĵ���;bit3:��������(0:1Mbps,1:2Mbps);bit2:1,���书��;bit0:�������Ŵ�������
#define STATUS          0x07  //״̬�Ĵ���;bit0:TX FIFO����־;bit3:1,��������ͨ����(���:6);bit4,�ﵽ�����ط�
                              //bit5:���ݷ�������ж�;bit6:���������ж�;
#define MAX_TX  	0x10  //�ﵽ����ʹ����ж�
#define TX_OK   	0x20  //TX��������ж�
#define RX_OK   	0x40  //���յ������ж�

#define TX_ADR_WIDTH    5   //5�ֽڵĵ�ַ���
#define RX_ADR_WIDTH    5   //5�ֽڵĵ�ַ���
#define TX_PLOAD_WIDTH  32  //20�ֽڵ��û����ݿ��
#define RX_PLOAD_WIDTH  32  //20�ֽڵ��û����ݿ��


#define RX_PW_P0        0x11  //��������ͨ��0��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P1        0x12  //��������ͨ��1��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P2        0x13  //��������ͨ��2��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P3        0x14  //��������ͨ��3��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P4        0x15  //��������ͨ��4��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P5        0x16  //��������ͨ��5��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define FIFO_STATUS     0x17  //FIFO״̬�Ĵ���;bit0,RX FIFO�Ĵ����ձ�־;bit1,RX FIFO����־;bit2,3,����
                              //bit4,TX FIFO�ձ�־;bit5,TX FIFO����־;bit6,1,ѭ��������һ���ݰ�.0,��ѭ��;

#define CHANAL 40		// ͨ��Ƶ��
u8 TX_ADDRESS[TX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01}; //���͵�ַ
u8 RX_ADDRESS[RX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01}; //���͵�ַ

void SPI_NRF_Init(void)    
{
  SPI_InitTypeDef  SPI_InitStructure;    
  GPIO_InitTypeDef GPIO_InitStructure;    
      
 /*ʹ�� GPIOB,GPIOD,���ù���ʱ��*/   
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE|RCC_APB2Periph_AFIO, ENABLE);    
   
 /*ʹ�� SPI1 ʱ��*/   
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);    
    
    /*���� SPI_NRF_SPI �� SCK,MISO,MOSI ���ţ�GPIOA^5,GPIOA^6,GPIOA^7 */   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;    
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;    
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //���ù���    
   GPIO_Init(GPIOA, &GPIO_InitStructure);      
    
   /*���� SPI_NRF_SPI �� CE ���ţ�GPIOA^2 �� SPI_NRF_SPI �� CSN ����: NSS GPIOA^1*/   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_1;    
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;    
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    
   GPIO_Init(GPIOA, &GPIO_InitStructure);     
    
    /*���� SPI_NRF_SPI ��IRQ ���ţ�GPIOA^3*/   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;    
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;    
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;  //��������    
   GPIO_Init(GPIOA, &GPIO_InitStructure);     
               
   /* �����Զ���ĺ꣬�������� csn ���ţ�NRF �������״̬ */   
   NRF_CSN_HIGH();     
      
   SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //˫��ȫ˫��    
   SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                     //��ģʽ    
   SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                 //���ݴ�С 8 λ    
   SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                        //ʱ�Ӽ��ԣ�����ʱΪ��    
   SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;                      //�� 1 ��������Ч��������Ϊ����ʱ��    
   SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                         //NSS �ź����������    
   SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  //8 ��Ƶ��9MHz    
   SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                //��λ��ǰ    
   SPI_InitStructure.SPI_CRCPolynomial = 7;    
   SPI_Init(SPI1, &SPI_InitStructure);    
    
   /* Enable SPI1  */   
   SPI_Cmd(SPI1, ENABLE);    
 }   

 
u8 SPI_NRF_RW(u8 dat)    
{       
   /* �� SPI ���ͻ������ǿ�ʱ�ȴ� */   
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);  
      
   /* ͨ�� SPI2 ����һ�ֽ����� */   
  SPI_I2S_SendData(SPI1, dat);          
     
   /* �� SPI ���ջ�����Ϊ��ʱ�ȴ� */   
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); 
   
  /* Return the byte read from the SPI bus */   
  return SPI_I2S_ReceiveData(SPI1);    
}   

u8 SPI_NRF_WriteBuf(u8 reg ,u8 *pBuf,u8 bytes)    
{    
     u8 status,byte_cnt;    
     NRF_CE_LOW();    
     /*�õ� CSN��ʹ�� SPI ����*/   
     NRF_CSN_LOW();             
   
     /*���ͼĴ�����*/     
     status = SPI_NRF_RW(reg);     
        
      /*�򻺳���д������*/   
     for(byte_cnt=0;byte_cnt<bytes;byte_cnt++)    
        SPI_NRF_RW(*pBuf++);    //д���ݵ�������        
               
    /*CSN ���ߣ����*/   
    NRF_CSN_HIGH();             
      
    return (status);    //���� NRF24L01 ��״̬           
}   

u8 SPI_NRF_ReadBuf(u8 reg,u8 *pBuf,u8 bytes)
{
        u8 status, byte_cnt;

          NRF_CE_LOW();
        /*�õ�CSN��ʹ��SPI����*/
        NRF_CSN_LOW();
                
        /*���ͼĴ�����*/                
        status = SPI_NRF_RW(reg); 

        /*��ȡ����������*/
         for(byte_cnt=0;byte_cnt<bytes;byte_cnt++)                  
           pBuf[byte_cnt] = SPI_NRF_RW(NOP); //��NRF24L01��ȡ����  

         /*CSN���ߣ����*/
        NRF_CSN_HIGH();        
                
        return status;                //���ؼĴ���״ֵ̬
}

u8 SPI_NRF_WriteReg(u8 reg,u8 dat)
{
        u8 status;
         NRF_CE_LOW();
        /*�õ�CSN��ʹ��SPI����*/
    NRF_CSN_LOW();
                                
        /*��������Ĵ����� */
        status = SPI_NRF_RW(reg);
                 
         /*��Ĵ���д������*/
    SPI_NRF_RW(dat); 
                  
        /*CSN���ߣ����*/           
          NRF_CSN_HIGH();        
                
        /*����״̬�Ĵ�����ֵ*/
           return(status);
}

u8 SPI_NRF_ReadReg(u8 reg)
{
        u8 reg_val;

        NRF_CE_LOW();
        /*�õ�CSN��ʹ��SPI����*/
        NRF_CSN_LOW();
                                
           /*���ͼĴ�����*/
        SPI_NRF_RW(reg); 

         /*��ȡ�Ĵ�����ֵ */
        reg_val = SPI_NRF_RW(NOP);
                    
           /*CSN���ߣ����*/
        NRF_CSN_HIGH();                
           
        return reg_val;
}        

u8 NRF_Check(void)    
{    
    u8 buf[5]={0xC2,0xC2,0xC2,0xC2,0xC2};    
    u8 buf1[5]={0};    
    u8 i;     
         
    /*д�� 5 ���ֽڵĵ�ַ.  */      
    SPI_NRF_WriteBuf(NRF_WRITE_REG+TX_ADDR,buf,5);    
   
    /*����д��ĵ�ַ */   
    SPI_NRF_ReadBuf(TX_ADDR,buf1,5);     
         
    /*�Ƚ�*/                   
    for(i=0;i<5;i++)    
    {    
        if(buf1[i]!=0xC2)    
        break;    
    }     
               
    if(i==5)    
        return 0 ;        //MCU �� NRF �ɹ�����    
    else   
        return 1 ;        //MCU �� NRF ����������   
}   

void power_off()
{
    NRF_CE_LOW();
    SPI_NRF_WriteReg(NRF_WRITE_REG + CONFIG, 0x0D); 
    NRF_CE_HIGH();
    Delay(0xfffff);
}

void NRF_RX_Mode(void)    
   
{
		power_off();
    NRF_CE_LOW();       
   
   SPI_NRF_WriteBuf(NRF_WRITE_REG+RX_ADDR_P0,RX_ADDRESS,RX_ADR_WIDTH);//д RX �ڵ��ַ    
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+EN_AA,0x01);    //ʹ��ͨ�� 0 ���Զ�Ӧ��        
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+EN_RXADDR,0x01);//ʹ��ͨ�� 0 �Ľ��յ�ַ        
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+RF_CH,CHANAL);      //���� RF ͨ��Ƶ��        
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+RX_PW_P0,RX_PLOAD_WIDTH);//ѡ��ͨ�� 0����Ч���ݿ��          
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x27); //���� TX �������,0db����,2Mbps,���������濪��       
   
   SPI_NRF_WriteReg(NRF_WRITE_REG+CONFIG, 0x0f);  //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ     
   
/*CE ���ߣ��������ģʽ*/     
  NRF_CE_HIGH();     
	Delay(0xffff);
}      

void NRF_TX_Mode(void)
{  
		power_off();
        NRF_CE_LOW();                

   SPI_NRF_WriteBuf(NRF_WRITE_REG+TX_ADDR,TX_ADDRESS,TX_ADR_WIDTH);    //дTX�ڵ��ַ 

   SPI_NRF_WriteBuf(NRF_WRITE_REG+RX_ADDR_P0,RX_ADDRESS,RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK   

   SPI_NRF_WriteReg(NRF_WRITE_REG+EN_AA,0x01);     //ʹ��ͨ��0���Զ�Ӧ��    

   SPI_NRF_WriteReg(NRF_WRITE_REG+EN_RXADDR,0x01); //ʹ��ͨ��0�Ľ��յ�ַ  

   SPI_NRF_WriteReg(NRF_WRITE_REG+SETUP_RETR,0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��

   SPI_NRF_WriteReg(NRF_WRITE_REG+RF_CH,CHANAL);       //����RFͨ��ΪCHANAL

   SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x27);  //����TX�������,0db����,2Mbps,���������濪��   
        
   SPI_NRF_WriteReg(NRF_WRITE_REG+CONFIG,0x0e);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�

/*CE���ߣ����뷢��ģʽ*/        
  NRF_CE_HIGH();
    Delay(0xffff); //CEҪ����һ��ʱ��Ž��뷢��ģʽ
}

/*
* ��������NRF_Tx_Dat
* ����  ��������NRF�ķ��ͻ�������д������
* ����  ��txBuf���洢�˽�Ҫ���͵����ݵ����飬�ⲿ����        
* ���  �����ͽ�����ɹ�����TXDS,ʧ�ܷ���MAXRT��ERROR
* ����  ���ⲿ����
*/ 
u8 NRF_Tx_Dat(u8 *txbuf)
{
        u8 state;  

         /*ceΪ�ͣ��������ģʽ1*/
        NRF_CE_LOW();

        /*д���ݵ�TX BUF ��� 32���ֽ�*/                                                
   SPI_NRF_WriteBuf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);

      /*CEΪ�ߣ�txbuf�ǿգ��������ݰ� */   
         NRF_CE_HIGH();
                  
          /*�ȴ���������ж� */                            
        while(NRF_Read_IRQ()!=0);         
        
        /*��ȡ״̬�Ĵ�����ֵ */                              
        state = SPI_NRF_ReadReg(STATUS);

         /*���TX_DS��MAX_RT�жϱ�־*/                  
        SPI_NRF_WriteReg(NRF_WRITE_REG+STATUS,state);         

        SPI_NRF_WriteReg(FLUSH_TX,NOP);    //���TX FIFO�Ĵ��� 

         /*�ж��ж�����*/    
        if(state&MAX_TX)                     //�ﵽ����ط�����
            return MAX_TX; 

        else if(state&TX_OK)                  //�������
                         return TX_OK;
         else                                                  
                        return ERROR;                 //����ԭ����ʧ��
} 


/*
* ��������NRF_Rx_Dat
* ����  �����ڴ�NRF�Ľ��ջ������ж�������
* ����  ��rxBuf�����ڽ��ո����ݵ����飬�ⲿ����        
* ���  �����ս����
* ����  ���ⲿ����
*/ 
u8 NRF_Rx_Dat(u8 *rxbuf)
{
        u8 state; 
				int i=0;
        NRF_CE_HIGH();         //�������״̬
         /*�ȴ������ж�*/
        while(NRF_Read_IRQ()!=0 && i++<5000); 
				if (i==5000)
					return ERROR;
        
        NRF_CE_LOW();           //�������״̬
        /*��ȡstatus�Ĵ�����ֵ  */               
        state=SPI_NRF_ReadReg(STATUS);
         
        /* ����жϱ�־*/      
        SPI_NRF_WriteReg(NRF_WRITE_REG+STATUS,state);

        /*�ж��Ƿ���յ�����*/
        if(state&RX_OK)                                 //���յ�����
        {
          SPI_NRF_ReadBuf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//��ȡ����
             SPI_NRF_WriteReg(FLUSH_RX,NOP);          //���RX FIFO�Ĵ���
          return RX_OK; 
        }
        else    
                return ERROR;                    //û�յ��κ�����
}

typedef struct
{
	short mag_x;
	short mag_y;
	short mag_z;
	
	short accel_x;
	short accel_y;
	short accel_z;
	
	short temperature1;
	
	short gyro_x;
	short gyro_y;
	short gyro_z;
	
	long temperature2;
	long pressure;
	
} sensor_data;


void swap(void *p, int size)
{
	int i;
	u8 *pp = (u8*)p;
	u8 tmp;
	for(i=0; i<size/2; i++)
	{
		tmp = pp[i];
		pp[i] = pp[size-1-i];
		pp[size-1-i] = tmp;
	}
}
/**
  * @brief  Main program.
  * @param  None
  * @retval : None
  */
#define	BMP085_SlaveAddress 0xee
#define OSS 0

short I2C_Double_Read(u8 slave, u8 reg)
{
	short o;
	I2C_ReadReg(slave, reg, (u8*)&o, 2);
	swap(&o, 2);
	
	return o;
}

int main(void)
{
	int i=0;
	int result;
	float avg;
	u8 data[32] = {0};
	
	/* USART1 config */
	USART1_Config();
	//I2C_Configuration();
	SPI_NRF_Init();
	printf("NRF_Check() = %d\r\n", NRF_Check());


	printf("I2C init\r\n");
	I2C_init(0x30);
	printf("I2C init OK\r\n");

	do
	{
		short ac1;
		short ac2; 
		short ac3; 
		unsigned short ac4;
		unsigned short ac5;
		unsigned short ac6;
		short b1; 
		short b2;
		short mb;
		short mc;
		short md;		

		printf("start BMP085\r\n");
		
		ac1 = I2C_Double_Read(BMP085_SlaveAddress, 0xAA);
		ac2 = I2C_Double_Read(BMP085_SlaveAddress, 0xAC);
		ac3 = I2C_Double_Read(BMP085_SlaveAddress, 0xAE);
		ac4 = I2C_Double_Read(BMP085_SlaveAddress, 0xB0);
		ac5 = I2C_Double_Read(BMP085_SlaveAddress, 0xB2);
		ac6 = I2C_Double_Read(BMP085_SlaveAddress, 0xB4);
		b1 =  I2C_Double_Read(BMP085_SlaveAddress, 0xB6);
		b2 =  I2C_Double_Read(BMP085_SlaveAddress, 0xB8);
		mb =  I2C_Double_Read(BMP085_SlaveAddress, 0xBA);
		mc =  I2C_Double_Read(BMP085_SlaveAddress, 0xBC);
		md =  I2C_Double_Read(BMP085_SlaveAddress, 0xBE);

		printf("BMP085 initialized, ac1=%d,ac2=%d,ac3=%d,ac4=%d,ac5=%d,ac6=%d,b1=%d,b2=%d,mb=%d,mc=%d,md=%d\r\n", ac1, ac2, ac3, ac4, ac5, ac6, b1, b2, mb, mc, md);
		
		while(1)
		{
			long x1, x2, b5, b6, x3, b3, p;
			unsigned long b4, b7;
			long  temperature;
			long  pressure;
					
			I2C_WriteReg(BMP085_SlaveAddress, 0xF4, 0x2E);
			Delay(0xfffff);			
			temperature = I2C_Double_Read(BMP085_SlaveAddress, 0xF6);
			I2C_WriteReg(BMP085_SlaveAddress, 0xF4, 0x34 + (OSS << 6));
			Delay(0xfffff);
			pressure = I2C_Double_Read(BMP085_SlaveAddress, 0xF6);
			pressure &= 0x0000FFFF;
			
			x1 = ((long)temperature - ac6) * ac5 >> 15;
			x2 = ((long) mc << 11) / (x1 + md);
			b5 = x1 + x2;
			temperature = (b5 + 8) >> 4;
			
			b6 = b5 - 4000;
			x1 = (b2 * (b6 * b6 >> 12)) >> 11;
			x2 = ac2 * b6 >> 11;
			x3 = x1 + x2;
			b3 = (((long)ac1 * 4 + x3) + 2)/4;
			x1 = ac3 * b6 >> 13;
			x2 = (b1 * (b6 * b6 >> 12)) >> 16;
			x3 = ((x1 + x2) + 2) >> 2;
			b4 = (ac4 * (unsigned long) (x3 + 32768)) >> 15;
			b7 = ((unsigned long) pressure - b3) * (50000 >> OSS);
			if( b7 < 0x80000000)
				p = (b7 * 2) / b4 ;
			else  
				p = (b7 / b4) * 2;
			x1 = (p >> 8) * (p >> 8);
			x1 = (x1 * 3038) >> 16;
			x2 = (-7357 * p) >> 16;
			pressure = p + ((x1 + x2 + 3791) >> 4);
			
			
			
			printf("I2C data = %d, %d\r\n", temperature, pressure);
		}
	}while(0);
	
	NRF_RX_Mode();
	while(1)
	{
		//i = NRF_Tx_Dat(data);
		//NRF_TX_Mode();
		//printf("NRF_Tx_Dat(%d) = %d\r\n", i++, NRF_Tx_Dat(data));

		result = NRF_Rx_Dat(data);
		//printf("NRF_Rx_Dat(%d) = %d\r\n", i++, result);
		
		if (result== RX_OK)
		{
				sensor_data *p = (sensor_data*)data;
			
			swap(&p->mag_x, 2);
			swap(&p->mag_y, 2);
			swap(&p->mag_z, 2);

			swap(&p->accel_x, 2);
			swap(&p->accel_y, 2);
			swap(&p->accel_z, 2);

			swap(&p->temperature1, 2);

			swap(&p->gyro_x, 2);
			swap(&p->gyro_y, 2);
			swap(&p->gyro_z, 2);

			swap(&p->pressure, 4);
			swap(&p->temperature2, 4);
			
			printf("HMC5883=%d,%d,%d, ACCEL=%d,%d,%d, TEMP1 = %d, GYRO=%d,%d,%d, TEMP2=%d, pressure=%d\r\n",
			p->mag_x, p->mag_y, p->mag_z,
			p->accel_x, p->accel_y, p->accel_z, p->temperature1, p->gyro_x, p->gyro_y, p->gyro_z, p->temperature2, p->pressure);
			
			//printf("data=");
			//for(i=0; i<32; i++)
			//	printf("%02x,", data[i]);
			//printf("\r\n");
		}
	}
	
	/* enable adc1 and config adc1 to dma mode */
	ADC1_Init();
	
	printf("\r\n -------����һ��ADCʵ��------\r\n");
	
	while (1)
	{
		avg = 0;
		for(i=0;i<200;i++)
		{
			ADC_ConvertedValueLocal =(float) ADC_ConvertedValue/4095*3.374*2.957; // ��ȡת����ADֵ
			avg += ADC_ConvertedValueLocal;
			Delay(0xff);                              // ��ʱ 
		}
		
		avg /= 200.0f;
	
		printf("The current AD value = %f V, The current AD value = 0x%04X \r\n",avg, ADC_ConvertedValue);
		
		
		// I2C test
		I2C_ReadTmp();
		printf("I2C result=%02X", I2c_Buf[1]);
	}
}
/******************* (C) COPYRIGHT 2012 ***************** *****END OF FILE************/
