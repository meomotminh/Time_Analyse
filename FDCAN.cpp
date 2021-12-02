/************************************************************************/
/*                            FDCAN SECTION                             */
/************************************************************************/
#include "FDCAN.h"



void Set_Freq(can_t *obj, const can_pinmap_t *pinmap, int hz)
{

    MBED_ASSERT((int)pinmap->peripheral != NC);


    #if defined(__HAL_RCC_FDCAN1_CLK_ENABLE)
        __HAL_RCC_FDCAN1_CLK_ENABLE_();
    #else 
        __HAL_RCC_FDCAN_CLK_ENABLE();
    #endif

    
    if (pinmap->peripheral == CAN_1) {       
        obj->index = 0;
    }
    #if defined(FDCAN2_BASE)
    else if (pinmap->peripheral == CAN_2) {
        
        obj->index = 1;
    }
    #endif
    
    else {
        error("can_init wrong instance\n");
        return;
    }

    //Serial.println("Before Set Frequency!\n");
    
    // Set Frequency
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    #if (defined RCC_PERIPHCLK_FDCAN1)
        RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN1;
        RCC_PeriphClkInit.Fdcan1ClockSelection = RCC_FDCAN1CLKSOURCE_PLL1;
    #else 
        //Serial.println("Enter here!");
        RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        RCC_PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
    #endif

    
    
   #if defined(DUAL_CORE) && (TARGET_STM32H7)
    while (LL_HSEM_1StepLock(HSEM, CFG_HW_RCC_SEMID)) {
    }
   #endif /* DUAL_CORE */
    if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit) != HAL_OK) {
        //Serial.println("HAL_RCCEx_PeriphCLKConfig error\n");
    } else {
        //Serial.println("HAL_RCCEx_PeriphCLKConfig OK!");
    }
    #if defined(DUAL_CORE) && (TARGET_STM32H7)
    LL_HSEM_ReleaseLock(HSEM, CFG_HW_RCC_SEMID, HSEM_CR_COREID_CURRENT);
    #endif /* DUAL_CORE */


    // Config CAN pins
    pin_function(pinmap->rd_pin, pinmap->rd_function);
    pin_function(pinmap->td_pin, pinmap->td_function);

    // Add pull-ups
    pin_mode(pinmap->rd_pin, PullUp);
    pin_mode(pinmap->td_pin, PullUp);

    //digitalWrite(pinmap->td_pin, HIGH);         

    // Default Values
    obj->CanHandle.Instance = (FDCAN_GlobalTypeDef *)pinmap->peripheral;
      //Serial.println(obj);
      //Serial.print("Instance:"); Serial.println(pinmap->peripheral);
      
    

    #if (defined TARGET_STM32H7)
        PLL1_ClocksTypeDef pll1_clocks;
        HAL_RCCEx_GetPLL1ClockFreq(&pll1_clocks);
        //Serial.print("Hier Freq:"); Serial.println(pll1_clocks.PLL1_Q_Frequency);
        int ntq = pll1_clocks.PLL1_Q_Frequency / hz;  // 320

    #else
    #if (defined RCC_PERIPHCLK_FDCAN1)
        //Serial.println("Hier 2");  
        int ntq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN1) / hz;
    #else
        //Serial.println("Hier 2");  
        int ntq = HALL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN) / hz;
    #endif
    #endif

    //Serial.print("ntq:");  Serial.println(ntq);

    

    int nominalPrescaler = 1;
while (!IS_FDCAN_NOMINAL_TSEG1(ntq / nominalPrescaler)){ // > 1 && < 256
  nominalPrescaler++;   // = 2
  if (!IS_FDCAN_NOMINAL_PRESCALER(nominalPrescaler)){
    //Serial.println("Could not determine nominalPrescaler. Bad clock value\n");
  }
}
  
  ntq = ntq / nominalPrescaler; // = 320/2 = 160
  
  // Create CAN Handle init object
  obj->CanHandle.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  obj->CanHandle.Init.Mode = FDCAN_MODE_NORMAL;
  obj->CanHandle.Init.AutoRetransmission = DISABLE;
  obj->CanHandle.Init.TransmitPause = ENABLE;
  obj->CanHandle.Init.ProtocolException = DISABLE;
  
  obj->CanHandle.Init.NominalPrescaler = nominalPrescaler;
  obj->CanHandle.Init.NominalTimeSeg1 = ntq * 0.75;
  obj->CanHandle.Init.NominalTimeSeg2 = ntq - 1 - obj->CanHandle.Init.NominalTimeSeg1;
  obj->CanHandle.Init.NominalSyncJumpWidth = obj->CanHandle.Init.NominalTimeSeg2;
  // not used, only in FD CAN  
  obj->CanHandle.Init.DataPrescaler = 0x1;
  obj->CanHandle.Init.DataSyncJumpWidth = 0x1;
  obj->CanHandle.Init.DataTimeSeg1 = 0x1;
  obj->CanHandle.Init.DataTimeSeg2 = 0x1;
  
  // RAM offset
  obj->CanHandle.Init.MessageRAMOffset = 0;
  
   // fiCanHandle.lters
  obj->CanHandle.Init.StdFiltersNbr = 128;
  obj->CanHandle.Init.ExtFiltersNbr = 64;
  
  obj->CanHandle.Init.RxFifo0ElmtsNbr = 3;
  obj->CanHandle.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  obj->CanHandle.Init.RxFifo1ElmtsNbr = 0;
  obj->CanHandle.Init.RxBuffersNbr = 0;
  obj->CanHandle.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  obj->CanHandle.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  
  obj->CanHandle.Init.TxEventsNbr = 24;
  obj->CanHandle.Init.TxBuffersNbr = 0;
  obj->CanHandle.Init.TxFifoQueueElmtsNbr = 24;
  obj->CanHandle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  obj->CanHandle.Init.TxElmtSize = FDCAN_DATA_BYTES_8;

  //Serial.print("Nominal Prescaler:"); Serial.println(nominalPrescaler);
  //Serial.print("ntq:"); Serial.println(ntq);
  //Serial.print("Nominal TimeSeg1:"); Serial.println(obj->CanHandle.Init.NominalTimeSeg1);
  //Serial.print("Nominal TimeSeg2:"); Serial.println(obj->CanHandle.Init.NominalTimeSeg2);
  //Serial.print("SyncJumpWidth:"); Serial.println(obj->CanHandle.Init.NominalSyncJumpWidth);

  //Trd_internal_init(obj);
}

void Set_CAN_Pin(can_t *obj, PinName rd, PinName td, int hz){
    CANName can_rd = (CANName)pinmap_peripheral(rd, PinMap_CAN_RD);
    CANName can_td = (CANName)pinmap_peripheral(td, PinMap_CAN_TD);
    int peripheral = (int)pinmap_merge(can_rd, can_td);

    int function_rd = (int)pinmap_find_function(rd, PinMap_CAN_RD);
    int function_td = (int)pinmap_find_function(td, PinMap_CAN_TD);

    const can_pinmap_t static_pinmap = {peripheral, rd, function_rd, td, function_td};

    Set_Freq(obj, &static_pinmap, hz);
}


void Trd_internal_init(can_t *obj){
  // call HAL init function
  if (HAL_FDCAN_Init(&obj->CanHandle) != HAL_OK){
    Serial.println("HAL_FDCAN_Init error\n");
  } else {
    Serial.println("HAL_FDCAN_Init success!");
  }


  /**
   * Config Timestamp & Enable Timestamp
   * **/
  
  if (HAL_FDCAN_ConfigTimestampCounter(&obj->CanHandle, FDCAN_TIMESTAMP_PRESC_2) != HAL_OK){
    Serial.println("HAL_FDCAN_ConfigTimestampCounter error");
  } else {
    Serial.println("HAL_FDCAN_ConfigTimestampCounter OK");
  }

  if (HAL_FDCAN_EnableTimestampCounter(&obj->CanHandle, FDCAN_TIMESTAMP_INTERNAL) != HAL_OK){
    Serial.println("HAL_FDCAN_EnableTimestampCounter error");
  } else {
    Serial.println("HAL_FDCAN_EnableTimestampCounter OK");
  }
  

  /**
   * Config & Enable TT operation
   * **/
  FDCAN_TT_ConfigTypeDef TT_Config;
  TT_Config.OperationMode = FDCAN_TT_COMMUNICATION_LEVEL2;  // not external sync
  
  
  //TT_Config.GapEnable = FDCAN_STRICTLY_TT_OPERATION;  
  TT_Config.TimeMaster = FDCAN_TT_POTENTIAL_MASTER;  
  TT_Config.InitRefTrigOffset = 0;  
  TT_Config.EvtTrigPolarity = FDCAN_TT_EVT_TRIG_POL_RISING;
  TT_Config.BasicCyclesNbr = FDCAN_TT_CYCLES_PER_MATRIX_1;
  

  TT_Config.ExpTxTrigNbr = 1;
  TT_Config.TURNumerator = 0xFF;       // Must set smaller -> faster
  TT_Config.TURDenominator = 0x1;         // Must set
  TT_Config.TriggerMemoryNbr = 1;
  
  TT_Config.StopWatchTrigSel = FDCAN_TT_STOP_WATCH_TRIGGER_0; // TIM2
  TT_Config.EventTrigSel = FDCAN_TT_EVENT_TRIGGER_1;  // TIM3

  /*
  if (HAL_FDCAN_TT_ConfigOperation(&obj->CanHandle, &TT_Config) != HAL_OK){
    Serial.println("HAL_FDCAN_TT_ConfigOperation error");
  } else {
    Serial.println("HAL_FDCAN_TT_ConfigOperation OK");
  }*/

  /**
   * Config Reference message
   * **/
  /*
  if (HAL_FDCAN_TT_ConfigReferenceMessage(&obj->CanHandle, FDCAN_STANDARD_ID, 0x641, FDCAN_TT_REF_MESSAGE_NO_PAYLOAD) != HAL_OK){
    Serial.println("HAL_FDCAN_TT_ConfigReferenceMessage error");
  } else {
    Serial.println("HAL_FDCAN_TT_ConfigReferenceMessage OK");
  }*/

  /**
   * Config Trigger to send reference message
   * **/
  /*
  FDCAN_TriggerTypeDef sTriggerConfig;
  sTriggerConfig.TriggerIndex = 0;  // 0 - 63
  sTriggerConfig.TimeMark = 0xFF; // 0 - 0xFFFF
  sTriggerConfig.RepeatFactor = FDCAN_TT_REPEAT_EVERY_CYCLE; // FDCAN_TT_Repeat_Factor
  //sTriggerConfig.StartCycle = 
  sTriggerConfig.TmEventInt = FDCAN_TT_TM_GEN_INTERNAL_EVENT; // internal event is generated when trigger become active
  sTriggerConfig.TmEventExt = FDCAN_TT_TM_GEN_EXTERNAL_EVENT; // external event is generated when trigger become active
  sTriggerConfig.TriggerType = FDCAN_TT_TX_REF_TRIGGER; // transmit reference message in external event-synchronized time-triggered operation
  //sTriggerConfig.FilterType = FDCAN_STANDARD_ID;
  //sTriggerConfig.TxBufferIndex = 
  //sTriggerConfig.FilterIndex = 
  
  if (HAL_FDCAN_TT_ConfigTrigger(&obj->CanHandle, &sTriggerConfig) != HAL_OK){
    Serial.println("HAL_FDCAN_TT_ConfigTrigger error");
  } else {
    Serial.println("HAL_FDCAN_TT_ConfigTrigger OK");
  }*/


  /***
   * Config Timestamp counter
  ***/
  if (HAL_FDCAN_ConfigTimestampCounter(&obj->CanHandle, FDCAN_TIMESTAMP_PRESC_2) != HAL_OK){
    Serial.println("HAL_FDCAN_ConfigTimestampCounter error!");
  } else {
    Serial.println("HAL_FDCAN_ConfigTimestampCounter OK!");
  }
  /***
   * Enable Timestamp counter
  ***/
  if (HAL_FDCAN_EnableTimestampCounter(&obj->CanHandle, FDCAN_TIMESTAMP_INTERNAL) != HAL_OK){
    Serial.println("HAL_FDCAN_EnableTimestampCounter error!");
  } else {
    Serial.println("HAL_FDCAN_EnableTimestampCounter OK!");
  }
  
  

  /***
  * Config Timeout Counter
  **/
  if (HAL_FDCAN_ConfigTimeoutCounter(&obj->CanHandle, FDCAN_TIMEOUT_CONTINUOUS, 0xFFFF) != HAL_OK){
    Serial.println("HAL_FDCAN_ConfigTimeoutCounter error!");
  } else {
    Serial.println("HAL_FDCAN_ConfigTimeoutCounter OK!");
  }

  /**
  * Enable Timeout Counter
  **/
  if (HAL_FDCAN_EnableTimeoutCounter(&obj->CanHandle) != HAL_OK){
    Serial.println("HAL_FDCAN_EnableTimeoutCounter error!");
  } else {
    Serial.println("HAL_FDCAN_EnableTimeoutCounter OK!");
  }  

}



/**
 * from can_mode of mbed OS target STM
 * 
 * **/
int my_can_mode(can_t *obj, CanMode mode)
{
  HAL_StatusTypeDef temp = HAL_FDCAN_Stop(&obj->CanHandle);

  if ( temp != HAL_OK){
    Serial.print("HAL_FDCAN_Stop:\t");
    Serial.println(temp);
  }

  switch (mode){
    case MODE_RESET:
      break;
    case MODE_NORMAL:
      obj->CanHandle.Init.Mode = FDCAN_MODE_NORMAL;
      break;
    case MODE_SILENT: // Bus Monitoring
      obj->CanHandle.Init.Mode = FDCAN_MODE_BUS_MONITORING;
      break;
    case MODE_TEST_GLOBAL: // External Loopback
    case MODE_TEST_LOCAL:
      obj->CanHandle.Init.Mode = FDCAN_MODE_EXTERNAL_LOOPBACK;
      break;
    case MODE_TEST_SILENT:
      obj->CanHandle.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
      break;
    default:
      return 0;
  }

  Trd_internal_init(obj);
  
  return 1;
}



