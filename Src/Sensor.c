#include "Sensor.h"

static IKS01A2_MOTION_SENSOR_Axes_t AccValue;
static IKS01A2_MOTION_SENSOR_Axes_t GyrValue;
static IKS01A2_MOTION_SENSOR_Axes_t MagValue;
static IKS01A2_MOTION_SENSOR_Axes_t MagOffset;

static volatile uint8_t MagCalRequest = 0;
HAL_StatusTypeDef status;
extern I2C_HandleTypeDef hi2c1;
static uint32_t MagTimeStamp = 0;
static uint8_t MagCalStatus = 0;

static volatile uint8_t SensorReadRequest = 0;

char lib_version[35];
int lib_version_len;
	
 void Init_Sensors(void)
{
  (void)IKS01A2_MOTION_SENSOR_Init(IKS01A2_LSM6DSL_0, MOTION_ACCELERO | MOTION_GYRO);
  (void)IKS01A2_MOTION_SENSOR_Init(IKS01A2_LSM303AGR_MAG_0, MOTION_MAGNETO);

//  (void)IKS01A2_ENV_SENSOR_Init(IKS01A2_HTS221_0, ENV_TEMPERATURE | ENV_HUMIDITY);
//  (void)IKS01A2_ENV_SENSOR_Init(IKS01A2_LPS22HB_0, ENV_PRESSURE);

}

	void lib_init()
	{
		/* Sensor Fusion API initialization function */
  MotionFX_manager_init();

  /* OPTIONAL */
  /* Get library version */
  MotionFX_manager_get_version(lib_version, &lib_version_len);
		 /* Enable magnetometer calibration */
  MotionFX_manager_MagCal_start(ALGO_PERIOD);
	/*Start timer */
	//HAL_TIM_Base_Start_IT(AlgoTimHandle);
	/*Start fusion calibration */
	 MotionFX_manager_start_9X();
		
		}
	
void Lib_Process(void)
{
	 for (;;)
  {
		 if (SensorReadRequest == 1U)
    {
      SensorReadRequest = 0;
			Accelero_Sensor_Handler(IKS01A2_LSM6DSL_0);
      Gyro_Sensor_Handler(IKS01A2_LSM6DSL_0);
      Magneto_Sensor_Handler(IKS01A2_LSM303AGR_MAG_0);
			 /* Sensor Fusion specific part */
      FX_Data_Handler();
		}
	}
}

void Lib_ReadRequest()
{
	//SensorReadRequest = 1;
	Accelero_Sensor_Handler(IKS01A2_LSM6DSL_0);
      Gyro_Sensor_Handler(IKS01A2_LSM6DSL_0);
      Magneto_Sensor_Handler(IKS01A2_LSM303AGR_MAG_0);
			 /* Sensor Fusion specific part */
      FX_Data_Handler();
}

 void FX_Data_Handler()
{
#if ((defined (USE_STM32F4XX_NUCLEO)) || (defined (USE_STM32L4XX_NUCLEO)) || (defined (USE_STM32L1XX_NUCLEO)))
  MFX_input_t data_in;
  MFX_input_t *pdata_in = &data_in;
  MFX_output_t data_out;
  MFX_output_t *pdata_out = &data_out;
#elif (defined (USE_STM32L0XX_NUCLEO))
  MFX_CM0P_input_t data_in;
  MFX_CM0P_input_t *pdata_in = &data_in;
  MFX_CM0P_output_t data_out;
  MFX_CM0P_output_t *pdata_out = &data_out;
#else
#error Not supported platform
#endif

        data_in.gyro[0] = (float)GyrValue.x * FROM_MDPS_TO_DPS;
        data_in.gyro[1] = (float)GyrValue.y * FROM_MDPS_TO_DPS;
        data_in.gyro[2] = (float)GyrValue.z * FROM_MDPS_TO_DPS;

        data_in.acc[0] = (float)AccValue.x * FROM_MG_TO_G;
        data_in.acc[1] = (float)AccValue.y * FROM_MG_TO_G;
        data_in.acc[2] = (float)AccValue.z * FROM_MG_TO_G;

        data_in.mag[0] = (float)MagValue.x * FROM_MGAUSS_TO_UT50;
        data_in.mag[1] = (float)MagValue.y * FROM_MGAUSS_TO_UT50;
        data_in.mag[2] = (float)MagValue.z * FROM_MGAUSS_TO_UT50;

        /* Run Sensor Fusion algorithm */
        BSP_LED_On(LED2);
        MotionFX_manager_run(pdata_in, pdata_out, MOTIONFX_ENGINE_DELTATIME);
        BSP_LED_Off(LED2);


}

/**
 * @brief  Handles the ACC axes data getting/sending
 * @param  Msg the ACC part of the stream
 * @param  Instance the device instance
 * @retval None
 */
 void Accelero_Sensor_Handler(uint32_t Instance)
{
    (void)IKS01A2_MOTION_SENSOR_GetAxes(Instance, MOTION_ACCELERO, &AccValue);

}

/**
 * @brief  Handles the GYR axes data getting/sending
 * @param  Msg the GYR part of the stream
 * @param  Instance the device instance
 * @retval None
 */
 void Gyro_Sensor_Handler(uint32_t Instance)
{
    (void)IKS01A2_MOTION_SENSOR_GetAxes(Instance, MOTION_GYRO, &GyrValue);

}

/**
 * @brief  Handles the MAG axes data getting/sending
 * @param  Msg the MAG part of the stream
 * @param  Instance the device instance
 * @retval None
 */
 void Magneto_Sensor_Handler(uint32_t Instance)
{
  float ans_float;
#if ((defined (USE_STM32F4XX_NUCLEO)) || (defined (USE_STM32L4XX_NUCLEO)) || (defined (USE_STM32L1XX_NUCLEO)))
  MFX_MagCal_input_t mag_data_in;
  MFX_MagCal_output_t mag_data_out;
#elif (defined (USE_STM32L0XX_NUCLEO))
  MFX_CM0P_MagCal_input_t mag_data_in;
  MFX_CM0P_MagCal_output_t mag_data_out;
#else
#error Not supported platform
#endif

    (void)IKS01A2_MOTION_SENSOR_GetAxes(Instance, MOTION_MAGNETO, &MagValue);


    if (MagCalStatus == 0U)
    {
      mag_data_in.mag[0] = (float)MagValue.x * FROM_MGAUSS_TO_UT50;
      mag_data_in.mag[1] = (float)MagValue.y * FROM_MGAUSS_TO_UT50;
      mag_data_in.mag[2] = (float)MagValue.z * FROM_MGAUSS_TO_UT50;

#if ((defined (USE_STM32F4XX_NUCLEO)) || (defined (USE_STM32L4XX_NUCLEO)) || (defined (USE_STM32L1XX_NUCLEO)))
      mag_data_in.time_stamp = (int)MagTimeStamp;
      MagTimeStamp += (uint32_t)ALGO_PERIOD;
#endif

      MotionFX_manager_MagCal_run(&mag_data_in, &mag_data_out);

#if ((defined (USE_STM32F4XX_NUCLEO)) || (defined (USE_STM32L4XX_NUCLEO)) || (defined (USE_STM32L1XX_NUCLEO)))
      if (mag_data_out.cal_quality == MFX_MAGCALGOOD)
#elif (defined (USE_STM32L0XX_NUCLEO))
      if (mag_data_out.cal_quality == MFX_CM0P_MAGCALGOOD)
#else
#error Not supported platform
#endif
      {
        MagCalStatus = 1;

        ans_float = (mag_data_out.hi_bias[0] * FROM_UT50_TO_MGAUSS);
        MagOffset.x = (int32_t)ans_float;
        ans_float = (mag_data_out.hi_bias[1] * FROM_UT50_TO_MGAUSS);
        MagOffset.y = (int32_t)ans_float;
        ans_float = (mag_data_out.hi_bias[2] * FROM_UT50_TO_MGAUSS);
        MagOffset.z = (int32_t)ans_float;

        /* Disable magnetometer calibration */
        MotionFX_manager_MagCal_stop(ALGO_PERIOD);
      }
    }

    MagValue.x = (int32_t)(MagValue.x - MagOffset.x);
    MagValue.y = (int32_t)(MagValue.y - MagOffset.y);
    MagValue.z = (int32_t)(MagValue.z - MagOffset.z);


}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//  if (htim->Instance == TIM3)
//  {
//    Lib_ReadRequest();
//  }
//}