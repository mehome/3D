#include "I2C.h"
#include "stm32f10x_i2c.h"

int I2C_init(u8 OwnAddress1)
{
	I2C_InitTypeDef I2C_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* PB10,11 SCL and SDA */  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);       

	I2C_DeInit(I2C2);
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = OwnAddress1;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;
	I2C_Cmd(I2C2, ENABLE);
	I2C_Init(I2C2, &I2C_InitStructure);

	I2C_AcknowledgeConfig(I2C2, ENABLE);
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	
	return 0;
}

int I2C_ReadReg(u8 SlaveAddress, u8 startRegister, u8*out, int count)
{
	int i;

	//��������Ƿ�æ
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)); 

	/*����1�ֽ�1 Ӧ��ģʽ*/ 
	I2C_AcknowledgeConfig(I2C2, ENABLE); 

	/* ������ʼλ  */ 
	I2C_GenerateSTART(I2C2, ENABLE); 

	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); /*EV5,��ģʽ*/ 

	/*����������ַ(д)*/ 
	I2C_Send7bitAddress(I2C2, SlaveAddress, I2C_Direction_Transmitter); 

	while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	/*����Pointer Register*/ 
	I2C_SendData(I2C2, startRegister);
	while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); /*�����ѷ���*/

	/*��ʼλ*/ 
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 

	/*����������ַ(��)*/ 
	I2C_Send7bitAddress(I2C2, SlaveAddress, I2C_Direction_Receiver);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); 

	for(i=0; i<count; i++)
	{
		/*Ϊ�����յ����һ���ֽں����һ��NACK���壬�ڶ������ڶ��������ֽ�֮��(�ڵ����ڶ���RxNE�¼�֮��)�������ACK λ�� 
		Ϊ�˲���һ��ֹͣ/����ʼ��������������ڶ������ڶ��������ֽ�֮��(�ڵ����ڶ���RxNE �¼�֮��)���� STOP/START λ�� 
		ֻ����һ���ֽ�ʱ���պ���EV6 ֮��(EV6_1ʱ�����ADDR֮��)Ҫ�ر�Ӧ���ֹͣ�����Ĳ���λ��*/ 
		if (i==count-1)
		{
			I2C_AcknowledgeConfig(I2C2, DISABLE); //���һλ��Ҫ�ر�Ӧ���
			I2C_GenerateSTOP(I2C2, ENABLE);   //����ֹͣλ
		}

		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)); /* EV7 */ 
		out[i] = I2C_ReceiveData(I2C2);
	}

	/*�ٴ�����Ӧ��ģʽ*/ 
	I2C_AcknowledgeConfig(I2C2, ENABLE);
	
	return 0;
}


int I2C_WriteReg(u8 SlaveAddress, u8 Register, u8 data)
{
	// wait for bus

	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));

	// start
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 

	// send slave address
	I2C_Send7bitAddress(I2C2, SlaveAddress, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	// send Register address
	I2C_SendData(I2C2, Register);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));


	// send data
	I2C_SendData(I2C2, data); 
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));


	// stop
	I2C_GenerateSTOP(I2C2, ENABLE);	
	return 0;
}