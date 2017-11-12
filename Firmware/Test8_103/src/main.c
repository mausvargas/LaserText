//stm32f103c8t6
#include "stm32f10x.h"
#include "vertical_mirror.h"
#include "poly_mirror.h"
#include "laser_controlling.h"
#include "main.h"
#include "lcd_worker.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __GNUC__   
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf  
     set to 'Yes') calls __io_putchar() */   
  //#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)   
#else   
  //#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)   
#endif /* __GNUC__ */ 


long tmp;



volatile uint16_t test_pwm = 2000;
extern volatile uint16_t encoder_period;
extern float bldc_vertical_frequency;
extern uint8_t buffer_switch_request;//Main software set this flag when buffer switching is need

uint32_t analyse_timer = 0;
uint32_t update_timer = 0;

uint16_t scanning_freq = 0;//Frequency of scanning (lines in second)


void InitAll( void);
void Delay( long Val);
void Delay_ms(uint32_t ms);
void SetupUSART1(void);
void Setup_clk(void);
void analyse_handler(void);
void image_update_handler(void);
void image_update_handler2(void);

void main(void)
{
  InitAll();//������������� ���������
  
  SysTick_Config(SystemCoreClock / 1000);//Tick every 1 ms
  
  Delay_ms(100);
  
  vertical_mirror_init_hardware();
  poly_mirror_init_hardware();
  init_laser_controlling();
  
  laser_turn_on();
  
  lcd_clear_framebuffer();

  while(1) 
  {
    analyse_handler();
    image_update_handler2();
  }
}

void analyse_handler(void)
{
  if (TIMER_ELAPSED(analyse_timer))
  {
    START_TIMER(analyse_timer, 100);
    
    scanning_freq = (uint16_t)(ENCODER_TIMER_FREQ / encoder_period);
    poly_mirror_set_pwm_period(test_pwm);
  }
}



void image_update_handler(void)
{
  static uint8_t counter = 0;
  static uint8_t x = 0;
  static uint8_t dir = 0;
  if (TIMER_ELAPSED(update_timer) && (buffer_switch_request == 0))
  {
    START_TIMER(update_timer, 50);
    
    lcd_clear_framebuffer();
    char tmp_str[32];
    //sprintf(tmp_str, "����������� �������� ��������: %d", counter);
    //lcd_draw_string(tmp_str, 0, 0, FONT_SIZE_8, 0);

    
    sprintf(tmp_str, "COUNTER: %d", counter);
    lcd_draw_string(tmp_str, 0, 0, FONT_SIZE_8, 0);
    lcd_draw_string("LASER PROJECTOR TEST", x, 8, FONT_SIZE_8, 0);
    
    counter++;
    buffer_switch_request = 1;
    
    
    if ((x > 50) || (x < 1)) 
      dir^= 1;
    if (dir == 1)
      x++;
    else
      x--;
  }
}

void image_update_handler2(void)
{
  uint8_t i;
  static uint8_t counter = 0;
  
  if (TIMER_ELAPSED(update_timer) && (buffer_switch_request == 0))
  {
    START_TIMER(update_timer, 50);
    
    lcd_clear_framebuffer();
    /*
    for (i=0; i<100; i++)
    {
      uint16_t rnd_val = (uint16_t)((6.0 * sin((float)(i + counter) / 3.0)) + 8.0);
      //uint16_t rnd_val = (i + counter) % 16;
      lcd_set_pixel(i, rnd_val);
    }
    */
    lcd_draw_string("GEEKTIMES.RU", 0, 0, FONT_SIZE_8, 0);
    lcd_draw_string("LASER PROJECTOR", 0, 8, FONT_SIZE_8, 0);
    
    
    counter++;
    buffer_switch_request = 1;
  }
}




void InitAll( void) 
{
  Setup_clk();
  
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  
  //��� ����������
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  return;
}


void Setup_clk(void)
{
  ErrorStatus HSEStartUpStatus;
  
  //����������� ������� ������������
  RCC_DeInit(); /* RCC system reset(for debug purpose) */
  RCC_HSEConfig(RCC_HSE_ON);/* Enable HSE */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();/* Wait till HSE is ready */
  
  if (HSEStartUpStatus == SUCCESS)
  {
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);/* Enable Prefetch Buffer */
    FLASH_SetLatency(FLASH_Latency_2);/* Flash 2 wait state */
    
    RCC_HCLKConfig(RCC_SYSCLK_Div1);/* HCLK = SYSCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1);/* PCLK2 = HCLK */
    RCC_PCLK1Config(RCC_HCLK_Div2);/* PCLK1 = HCLK / 2 */
    
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_3);//PLLCLK = 8MHz * 9 = 72 MHz <<<<<
    RCC_PLLCmd(ENABLE);/* Enable PLL */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){};/* Wait till PLL is ready */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);/* Select PLL as system clock source */
    while (RCC_GetSYSCLKSource() != 0x08){}/* Wait till PLL is used as system clock source */
  }
  SystemCoreClockUpdate();
  
}

void Delay_ms(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq (&RCC_Clocks);
  nCount=(RCC_Clocks.HCLK_Frequency/10000)*ms;
  for (; nCount!=0; nCount--);
}


void SetupUSART1(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* Enable GPIOA clock                                                   */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_USART1, ENABLE);
  
  /* Configure USART1 Rx (PA10) as input floating                         */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure USART1 Tx (PA9) as alternate function push-pull            */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  USART_InitStructure.USART_BaudRate = 9600;   
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;   
  USART_InitStructure.USART_StopBits = USART_StopBits_1;   
  USART_InitStructure.USART_Parity = USART_Parity_No ;   
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 
  
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);
  
}


int putchar(int c)
 {
    /* Write a character to the USART */   
  USART_SendData(USART1, (u8) c);   
   
  /* Loop until the end of transmission */   
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)   
  {   
  }      
  return c;    
}






