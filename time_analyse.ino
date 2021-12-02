#include "Arduino.h"
#include "mbed.h"
#include "FDCAN.h"

can_t my_can;

boolean sendMsg = false;
boolean receiveMsg = false;
boolean timeStamp = false;
boolean timestampVal = 0;
boolean timerElapse = false;

uint16_t time_array;
TIM_HandleTypeDef htimer6;
int timeDiff = 0;
char buffer[40];

uint32_t m_nStart;  // DEBUG stopwatch start cycle counter value
uint32_t m_nStop;   // DEBUG stopwatch stop cycle counter value

#define DEMCR_TRCENA  0x01000000

// Core debug register
#define DEMCR           (*((volatile uint32_t *)0xE000EDFC))
#define DWT_CTRL        (*(volatile uint32_t *)0xe0001000)
#define CYCCNTENA       (1<<0)
#define DWT_CYCCNT      ((volatile uint32_t *)0xE0001004)
#define CPU_CYCLES      *DWT_CYCCNT
#define CLK_SPEED       400000000 

#define STOPWATCH_START { m_nStart = *((volatile unsigned int *)0xE0001004);}
#define STOPWATCH_STOP  { m_nStop = *((volatile unsigned int *)0xE0001004);}


/* ------------------------------- CYCLE COUNT ------------------------------ */
static inline void stopwatch_reset(void)
{
    /* Enable DWT */
    DEMCR |= DEMCR_TRCENA; 
    *DWT_CYCCNT = 0;             
    /* Enable CPU cycle counter */
    DWT_CTRL |= CYCCNTENA;
}

static inline uint32_t stopwatch_getticks()
{
    return CPU_CYCLES;
}

static inline void stopwatch_delay(uint32_t ticks)
{
    uint32_t end_ticks = ticks + stopwatch_getticks();
    while(1)
    {
            if (stopwatch_getticks() >= end_ticks)
                    break;
    }
}

uint32_t CalcNanosecondsFromStopwatch(uint32_t nStart, uint32_t nStop)
{
    uint32_t nDiffTicks;
    uint32_t nSystemCoreTicksPerMicrosec;

    // Convert (clk speed per sec) to (clk speed per microsec)
    nSystemCoreTicksPerMicrosec = CLK_SPEED / 1000000;

    // Elapsed ticks
    nDiffTicks = nStop - nStart;

    // Elapsed nanosec = 1000 * (ticks-elapsed / clock-ticks in a microsec)
    return 1000 * nDiffTicks / nSystemCoreTicksPerMicrosec;
} 


/* ----------------------------- TIMER CALLBACK ----------------------------- */

void TIMER6_Init(){
  RCC_ClkInitTypeDef clkconfig;
  uint32_t uwTimClock, uwAPB1Prescaler = 0U;
  uint32_t uwPrescalerValue = 0U;
  uint32_t pFLatency;

  /* ----------------------- enable TIM interface clock ----------------------- */
  __HAL_RCC_TIM6_CLK_ENABLE();

  
  /* ------------------------- get clock configuration ------------------------ */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* --------------------------- get APB1 prescaler --------------------------- */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;
  sprintf(buffer,"wqAPB1Prescaler: %d\n", uwAPB1Prescaler);
  Serial.print(buffer);

  sprintf(buffer,"RCC_HCLK_DIV1: %d\n", RCC_HCLK_DIV1);
  Serial.print(buffer);

  /* --------------------------- compute TIM6 clock --------------------------- */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1){
    uwTimClock = HAL_RCC_GetPCLK1Freq();
  } else {
    Serial.println("Get here!");
    uwTimClock = 2*HAL_RCC_GetPCLK1Freq();
  }

  Serial.print("TIM6 Clock: "); Serial.println(uwTimClock);

  /* -- compute the prescaler value to have TIM6 counter clock equal to 1 MHz - */
  uwPrescalerValue = (uint32_t) ((uwTimClock / 1000000U) - 1U);

  /* ----------------------------- initialize TIM6 ---------------------------- */
  htimer6.Instance = TIM6; // Basic Timer

  /*
    Initialize TIMx peripheral as follow
    Period = [(TIM6CLK / 1000) - 1]
    Prescaler = (uwTimclock / 1000000 - 1)
    ClockDivision = 0

  */
  htimer6.Init.Period = (1000000U / 1000U) - 1;
  htimer6.Init.Prescaler = uwPrescalerValue;
  htimer6.Init.CounterMode = TIM_COUNTERMODE_UP;
  
  /* ----------------- initialize the TIM low level resources ----------------- */
  if (HAL_TIM_Base_Init(&htimer6) != HAL_OK){
    //Serial.println("HAL_TIM_Base_Init error!");
  } else {    
    //Serial.println("HAL_TIM_Base_Init OK!");
  }

  
  /* ------------------------- Activate TIM peripheral ------------------------ */
  if (HAL_TIM_Base_Start_IT(&htimer6) != HAL_OK){
    stopwatch_reset();
    STOPWATCH_START;
  } else {
    
  }

  NVIC_SetVector(TIM6_DAC_IRQn, (uint32_t)&TIM6_DAC_IRQHandler);
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 14U, 0U);
  // enable TIMx global interrupt
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

}

void TIM6_DAC_IRQHandler(void){
  
  HAL_TIM_IRQHandler(&htimer6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
  
  /* ----------------------- only trigger when alarm set ---------------------- */
  STOPWATCH_STOP;
  timerElapse = true;
  
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  while(!Serial);

  HAL_Init();

  

  TIMER6_Init();
  /*
  // GET SYSCLK SOURCE
  if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_CFGR_SWS_CSI){
    Serial.println("CSI source");
  } else if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_CFGR_SWS_HSI){
    Serial.println("HSI source");
  } else if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_CFGR_SWS_HSE){
    Serial.println("HSE source");
  } else if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_CFGR_SWS_PLL1){
    Serial.println("PLL source");
  }
  
  PLL1_ClocksTypeDef pll1_clocks;
  
  HAL_RCCEx_GetPLL1ClockFreq(&pll1_clocks);
  // get PLL1 clock freq
  sprintf(buffer,"PLL1 P for system clock freq: %d \n", pll1_clocks.PLL1_P_Frequency);
  Serial.print(buffer);
  sprintf(buffer,"PLL1 R for debug trace freq: %d \n", pll1_clocks.PLL1_R_Frequency);
  Serial.print(buffer);
  sprintf(buffer,"PLL1 Q for kernel clock freq: %d \n", pll1_clocks.PLL1_Q_Frequency);
  Serial.print(buffer);
  */
    
}

void loop() {
  // put your main code here, to run repeatedly:
  timeDiff = 0;


  
  if (timerElapse){
    timeDiff = CalcNanosecondsFromStopwatch(m_nStart, m_nStop);
    sprintf(buffer,"Delay measured is %d nanoseconds\n", timeDiff);
    Serial.print(buffer);
    TIM6->CNT = 0;
    stopwatch_reset();
    STOPWATCH_START;
    timerElapse = false;
  }
  
  
  
 
}
