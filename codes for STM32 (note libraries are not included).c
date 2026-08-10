#include "main.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ds18b20.h"
#include "onewire.h"
#include "dwt_delay.h"
#include <stdio.h>
#include <math.h>
#include "stm32f1xx.h"

char handledata[32];

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

extern uint8_t sensor_count;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);

int16_t readAnalogSignalfromchargeamp(uint32_t channel) {  /* read analog signal */
    ADC_ChannelConfTypeDef sConfig={0};
    sConfig.SamplingTime= ADC_SAMPLETIME_7CYCLES_5;
    sConfig.Channel = channel;
    sConfig.Rank= 1;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
            return HAL_ADC_GetValue(&hadc1);
        }
        return 2048;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();

  DWT_Init();
  DS18B20_ScanBus();
  ssd1306_Init();

  uint32_t last_time_OLEDdisplay= 0;
  uint32_t startingtime= 0;

  const float Vrefereadc= 3.212f;   /* Vreference to calculate Vrms */
  const float Radc= 4095.0f;     /* means the resolution of an ADC */
  const float blocDC= 0.004f;           /* blocking unwanted DC */


  float FilteroutALsignal1= 2048.0f;            /* filter noise */
  float FilteroutALsignal2= 2048.0f;
  float FilteroutALsignal3= 2048.0f;

  float squaredvoltagesum1= 0.0f;
  float squaredvoltagesum2= 0.0f;
  float squaredvoltagesum3= 0.0f;

  uint32_t countingSample_N= 0;

  float VoltageRMS1 = 0.0f;
  float VoltageRMS2 = 0.0f;
  float VoltageRMS3 = 0.0f;

   uint32_t last_sample_time=0;
   uint32_t last_led_time=0;
   uint32_t last_time_DS18B20_sensors=0;

   float lastAnalogsignal1=0.0f;
   float lastAnalogsignal2=0.0f;
   float lastAnalogsignal3=0.0f;

   float f1=0.0f;
   float f2=0.0f;
   float f3=0.0f;

   uint32_t  NzcCount1=0;
   uint32_t  NzcCount2=0;
   uint32_t  NzcCount3=0;

   float Cleanedf1 = 0.0f;
   float Cleanedf2 = 0.0f;
   float Cleanedf3 = 0.0f;

   float PCS= 400.0f;
   float Cf=4700.0f ;
   float mass = 0.144f;   /* piezoelectric sensor on electrodynamic unit  in kg */

   float acceleration1=0.0f;
   float acceleration2=0.0f;
   float acceleration3=0.0f;


  while (1)
  {

      uint32_t timenow=HAL_GetTick();


      if (timenow-last_led_time >= 20) {
          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
          last_led_time =timenow;
      }


      if (timenow - last_sample_time>= 1) {
           last_sample_time=timenow;

           if (countingSample_N== 0) {
        	   startingtime = timenow;
           }

           uint16_t s1 = readAnalogSignalfromchargeamp(ADC_CHANNEL_1);
           uint16_t s2 = readAnalogSignalfromchargeamp(ADC_CHANNEL_4);
           uint16_t s3 = readAnalogSignalfromchargeamp(ADC_CHANNEL_8);


           if (s1>500) HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9) ;
           else         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

           if (s2>500) HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_10) ;
           else         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

           if (s3>500) HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8) ;
           else         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);


           FilteroutALsignal1 = (blocDC*(float)s1)+((1.0f - blocDC)*FilteroutALsignal1);
           FilteroutALsignal2 = (blocDC*(float)s2)+((1.0f - blocDC)*FilteroutALsignal2);
           FilteroutALsignal3 = (blocDC*(float)s3)+((1.0f - blocDC)*FilteroutALsignal3);


           float DigitizedAnalogsignal1 = (float)s1-FilteroutALsignal1;
           float DigitizedAnalogsignal2 = (float)s2-FilteroutALsignal2;
           float DigitizedAnalogsignal3 = (float)s3-FilteroutALsignal3;


           squaredvoltagesum1 += (DigitizedAnalogsignal1*DigitizedAnalogsignal1) ;
           squaredvoltagesum2 += (DigitizedAnalogsignal2*DigitizedAnalogsignal2) ;
           squaredvoltagesum3 += (DigitizedAnalogsignal3*DigitizedAnalogsignal3) ;
           countingSample_N++;


               if ((DigitizedAnalogsignal1 >= 0.0f && lastAnalogsignal1 < 0.0f)||(DigitizedAnalogsignal1 < 0.0f && lastAnalogsignal1 >= 0.0f)) {
        	       NzcCount1 ++;
               }
               if ((DigitizedAnalogsignal2 >= 0.0f && lastAnalogsignal2 < 0.0f)||(DigitizedAnalogsignal2 < 0.0f && lastAnalogsignal2 >= 0.0f)) {
            	   NzcCount2 ++;
               }
               if ((DigitizedAnalogsignal3 >= 0.0f && lastAnalogsignal3 < 0.0f)||(DigitizedAnalogsignal3 < 0.0f && lastAnalogsignal3 >= 0.0f)) {
            	   NzcCount3 ++;
               }


               lastAnalogsignal1=DigitizedAnalogsignal1;
               lastAnalogsignal2=DigitizedAnalogsignal2;
               lastAnalogsignal3=DigitizedAnalogsignal3;

      }


      if (countingSample_N >= 5) {

    	   float StartTimetoEndTime = 0.005f;
           float insdEquat1 = squaredvoltagesum1/(float)countingSample_N;
           float insdEquat2 = squaredvoltagesum2/(float)countingSample_N;
           float insdEquat3 = squaredvoltagesum3/(float)countingSample_N;

           VoltageRMS1 = (sqrtf(insdEquat1)* Vrefereadc) / Radc;
           VoltageRMS2 = (sqrtf(insdEquat2) * Vrefereadc) / Radc;
           VoltageRMS3 = (sqrtf(insdEquat3) * Vrefereadc) / Radc;


           if (VoltageRMS1 < 0.04f)VoltageRMS1 = 0.0f;
           if (VoltageRMS2 < 0.04f)VoltageRMS2 = 0.0f;
           if (VoltageRMS3 < 0.04f)VoltageRMS3 = 0.0f;

           if (VoltageRMS1 < 0.01f) {
                f1 = 0.0f ;
            } else {
                f1 = ((float)  NzcCount1*50/ 2.0f)/ StartTimetoEndTime ;
            }

            if (VoltageRMS2 < 0.01f) {
                f2 = 0.0f ;
            } else {
                f2 = ((float)  NzcCount2*50/ 2.0f)/ StartTimetoEndTime ;
            }

            if (VoltageRMS3 < 0.01f) {
                f3= 0.0f ;
            } else {
                f3 = ((float)  NzcCount3*50/ 2.0f)/ StartTimetoEndTime ;
            }

            Cleanedf1 = (f1*0.06f)+((0.52f+0.42f)*Cleanedf1) ;
            Cleanedf2 = (f2*0.06f)+((0.52f+0.42f)*Cleanedf2) ;
            Cleanedf3 = (f3*0.06f)+((0.52f+0.42f)*Cleanedf3) ;

           float Sensitivity = PCS/Cf ;

           acceleration1 = VoltageRMS1/(Sensitivity * mass) ;  /* piezoelectric disc sensor acts as accelerometer*/
           acceleration2 = VoltageRMS2/(Sensitivity * mass) ;
           acceleration3 = VoltageRMS3/(Sensitivity * mass) ;

           NzcCount1 = 0;
           NzcCount2 = 0;
           NzcCount3 = 0;
           squaredvoltagesum1 = 0.0f; squaredvoltagesum2 = 0.0f; squaredvoltagesum3 = 0.0f;
           countingSample_N = 0;


      }


      if (timenow-last_time_DS18B20_sensors>= 750){
    	  last_time_DS18B20_sensors = timenow;
          OneWire_Reset();
          OneWire_WriteByte(0xCC);
          OneWire_WriteByte(0x44);
      }

      if (timenow-last_time_OLEDdisplay>= 250)
      {
    	  last_time_OLEDdisplay = timenow;
          ssd1306_Fill(Black);


          for (int i = 0; i < sensor_count; i++)
          {
              float temp = DS18B20_Read(sensors[i].rom);
              sprintf(handledata, "T%d:%.1f", i + 1, temp);
              ssd1306_SetCursor(85, (i * 14) + 26);
              ssd1306_WriteString(handledata, Font_6x8, White);
          }

          ssd1306_SetCursor(30, 18);
          ssd1306_WriteString("ms", Font_6x8, White);
          ssd1306_SetCursor(42, 16);
          ssd1306_WriteString("-2", Font_6x8, White);
          ssd1306_SetCursor(60, 18);
          ssd1306_WriteString("Hz", Font_6x8, White);
          sprintf(handledata, "a1|f1:%.1f", (float)acceleration1);
          ssd1306_SetCursor(0, 30);
          ssd1306_WriteString(handledata, Font_6x8, White);
          sprintf(handledata, "a2|f2:%.1f", (float)acceleration2);
          ssd1306_SetCursor(0, 42);
          ssd1306_WriteString(handledata, Font_6x8, White);
          sprintf(handledata, "a3|f3:%.1f", (float)acceleration3);
          ssd1306_SetCursor(0, 54);
          ssd1306_WriteString(handledata, Font_6x8, White);
          sprintf(handledata, "|%d", (int)Cleanedf1);
          ssd1306_SetCursor(54, 30);
          ssd1306_WriteString(handledata, Font_6x8, White);
          sprintf(handledata, "|%d", (int)Cleanedf2);
          ssd1306_SetCursor(54, 42);
          ssd1306_WriteString(handledata, Font_6x8, White);
          sprintf(handledata, "|%d", (int)Cleanedf3);
          ssd1306_SetCursor(54, 54);
          ssd1306_WriteString(handledata, Font_6x8, White);
          ssd1306_SetCursor(0, 0);
          ssd1306_WriteString("Vib| ", Font_6x8, White);
          ssd1306_SetCursor(68, 0);
          ssd1306_WriteString("Temp|", Font_6x8, White);
          ssd1306_SetCursor(73, 16);
          ssd1306_WriteString("     C ", Font_6x8, White);
          ssd1306_SetCursor(73, 12);
          ssd1306_WriteString("    .", Font_6x8, White);

          if((acceleration1 < 6.5)&&(acceleration2 < 6.5)&&(acceleration3 < 6.5))
            {
        	 ssd1306_SetCursor(20, 0);
        	 ssd1306_WriteString("|Normal", Font_6x8, White);
            }
          else
          {
        	  ssd1306_SetCursor(20, 0);
        	  ssd1306_WriteString("|Defect", Font_6x8, White);
            }


          ssd1306_UpdateScreen();

      }
  }
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  // FIXED: Changed initialized baseline speed to match a slower, noise-resistant profile
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */




