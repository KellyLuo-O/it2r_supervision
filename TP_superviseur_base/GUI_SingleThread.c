
#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif

#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"

#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "RTE_Components.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Board_Touch.h"                // ::Board Support:Touchscreen
#include "GUI.h"
#include "stdio.h"
#include "DIALOG.h"
#include "Board_LED.h"                  // ::Board Support:LED
#include "Driver_USART.h"               // ::CMSIS Driver:USART

extern ARM_DRIVER_USART Driver_USART6;

	
	char couple, vitesse, reservoir;
	int temperature;
	char feux;

	WM_HWIN hDlg;
	


#ifdef RTE_CMSIS_RTOS_RTX
extern uint32_t os_time;

uint32_t HAL_GetTick(void) { 
  return os_time; 
}
#endif


/*********************************************************************
* *
Externals
* **********************************************************************
*/
WM_HWIN CreateWindow(void);


/*----------------------------------------------------------------------------
 *      GUIThread: GUI Thread for Single-Task Execution Model
 *---------------------------------------------------------------------------*/
 
void GUIThread (void const *argument);              // thread function
osThreadId tid_GUIThread;                           // thread id
osThreadDef (GUIThread, osPriorityIdle, 1, 2048);   // thread object

int Init_GUIThread (void) {

  tid_GUIThread = osThreadCreate (osThread(GUIThread), NULL);
  if (!tid_GUIThread) return(-1);
  
  return(0);
}

/**
  * System Clock Configuration
  *   System Clock source            = PLL (HSE)
  *   SYSCLK(Hz)                     = 200000000
  *   HCLK(Hz)                       = 200000000
  *   AHB Prescaler                  = 1
  *   APB1 Prescaler                 = 4
  *   APB2 Prescaler                 = 2
  *   HSE Frequency(Hz)              = 25000000
  *   PLL_M                          = 25
  *   PLL_N                          = 400
  *   PLL_P                          = 2
  *   PLL_Q                          = 8
  *   VDD(V)                         = 3.3
  *   Main regulator output voltage  = Scale1 mode
  *   Flash Latency(WS)              = 6
  */
static void SystemClock_Config (void) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;  
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Activate the OverDrive to reach the 200 MHz Frequency */
  HAL_PWREx_EnableOverDrive();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
}



/**
  * Configure the MPU attributes
  */
static void MPU_Config (void) {
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xC0200000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}


/**
  * CPU L1-Cache enable
  */
static void CPU_CACHE_Enable (void) {

  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}


void GUIThread (void const *argument) {

	
	
	MPU_Config ();
	CPU_CACHE_Enable();                       /* Enable the CPU Cache           */
  HAL_Init();                               /* Initialize the HAL Library     */
  BSP_SDRAM_Init();                         /* Initialize BSP SDRAM           */
  SystemClock_Config();                     /* Configure the System Clock     */

  GUI_Init();
	Touch_Initialize();
	
	// Call creation function for the dialog
	hDlg = CreateWindow();
	
	/* Add GUI setup code here */

  while (1) {
    
    /* All GUI related activities might only be called from here */
		
		// mises à jour affichage
		GUI_Exec();
		
		GUI_Delay(10);
		GUI_X_ExecIdle();             /* Nothing left to do for the moment ... Idle processing */
  }
}



void UART_threadR (void const *argument);                             // thread function Transmit
osThreadId ID_UART_threadR;                                          // thread id
osThreadDef (UART_threadR, osPriorityNormal, 1, 0);                   // thread object

void UART_threadT (void const *argument);                             // thread function Transmit
osThreadId ID_UART_threadT;                                          // thread id
osThreadDef (UART_threadT, osPriorityNormal, 1, 0);                   // thread object


void usart_cb(uint32_t event)
{

		if ((event & ARM_USART_EVENT_RECEIVE_COMPLETE) == ARM_USART_EVENT_RECEIVE_COMPLETE) 
		{
			osSignalSet(ID_UART_threadR, 0x0002);
		}
		if ((event & ARM_USART_EVENT_TX_COMPLETE) == ARM_USART_EVENT_TX_COMPLETE) 
		{
			osSignalSet(ID_UART_threadT, 0x0002);
		}	
}

void Init_UART(void){
	Driver_USART6.Initialize(usart_cb);
	Driver_USART6.PowerControl(ARM_POWER_FULL);
	Driver_USART6.Control(	ARM_USART_MODE_ASYNCHRONOUS |
							ARM_USART_DATA_BITS_8		|
							ARM_USART_STOP_BITS_1		|
							ARM_USART_PARITY_NONE		|
							ARM_USART_FLOW_CONTROL_NONE,
							1528);   //36770
	Driver_USART6.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART6.Control(ARM_USART_CONTROL_RX,1);
}


// Tache reception UART

void UART_threadR (void const *argument) {

	unsigned char tab[1];
	while (1) 
		{
			Driver_USART6.Receive(tab,1);	
			osSignalWait(0x0002, osWaitForever);
			
			switch (tab[0] & 0xC0)
			{
				case 0x40:
					couple = (tab[0]&0x3F)*100/64;
					break;
				case 0x80:
					vitesse = (tab[0]&0x3F)*100/64;
					break;
				case 0xC0:
					reservoir = (tab[0]&0x3F)*100/64;
					break;			
				default : break;
			}
			WM_SendMessageNoPara(hDlg, WM_USER);
		}
}

void UART_threadT (void const *argument) {
	
	unsigned char tab[1];
	
	while (1) 
		{
			tab[0]=200-temperature;
			Driver_USART6.Send(tab,1);
			osSignalWait(0x0002, osWaitForever);
			
			tab[0] = 0xF0 | feux;
			Driver_USART6.Send(tab,1);
			osSignalWait(0x0002, osWaitForever);
			
			osDelay(100);
		}
}

/*********************************************************************
*
*       Main
*/
int main (void) {
	osKernelInitialize ();                    // initialize CMSIS-RTOS

  // initialize peripherals here
	Init_UART();
	
	
  // create 'thread' functions that start executing,
  Init_GUIThread();
	ID_UART_threadR = osThreadCreate (osThread(UART_threadR), NULL);
	ID_UART_threadT = osThreadCreate (osThread(UART_threadT), NULL);

  osKernelStart ();                         // start thread execution 
	
  osDelay(osWaitForever);
}

/*************************** End of file ****************************/

