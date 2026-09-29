/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : platform.c
 * @brief          : platform specific initialization functions and management
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
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

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

int __io_putchar(int ch)
{

  return ch;
}

void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

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

#ifdef USE_FULL_ASSERT
#pragma message "Full assertions: defined"
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
#else
#pragma message "Full assertions: not defined"
#endif /* USE_FULL_ASSERT */

static inline void ENABLE_ALL_GPIO_CLOCKS(void)
{
#ifdef GPIOA
  __HAL_RCC_GPIOA_CLK_ENABLE();
#endif
#ifdef GPIOB
  __HAL_RCC_GPIOB_CLK_ENABLE();
#endif
#ifdef GPIOC
  __HAL_RCC_GPIOC_CLK_ENABLE();
#endif
#ifdef GPIOD
  __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
  __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
  __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
  __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
  __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
#ifdef GPIOI
  __HAL_RCC_GPIOI_CLK_ENABLE();
#endif
#ifdef GPIOJ
  __HAL_RCC_GPIOJ_CLK_ENABLE();
#endif
#ifdef GPIOK
  __HAL_RCC_GPIOK_CLK_ENABLE();
#endif
}

void platform_init(void)
{
  HAL_Init();
  SystemClock_Config();
  ENABLE_ALL_GPIO_CLOCKS();
}