
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"



/* USER CODE BEGIN Includes */

// Lishui BLCD FOC Open Source Firmware
// Board uses IRS2003 half bridge drivers, this need inverted pulses for low-side Mosfets, deadtime is generated in driver
// This firmware bases on the ST user manual UM1052
// It uses the OpenSTM32 workbench (SW4STM32 toolchain)
// Basic peripheral setup was generated with CubeMX

#include "print.h"
#include "FOC.h"
#include "config.h"
#include "eeprom.h"


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
  #include "display_kingmeter.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  #include "display_bafang.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
  #include "display_kunteng.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
  #include "display_ebics.h"
#endif


#include <arm_math.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;


UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/


uint32_t ui32_tim1_counter=0;
uint32_t ui32_tim3_counter=0;
uint8_t ui8_hall_state=0;
uint8_t ui8_hall_state_old=0;
uint8_t ui8_hall_case =0;
uint16_t ui16_tim2_recent=0;
uint16_t ui16_timertics=5000; 					//timertics between two hall events for 60Â° interpolation
uint16_t ui16_throttle;
uint16_t ui16_brake_adc;
uint32_t ui32_throttle_cumulated;
uint32_t ui32_brake_adc_cumulated;
uint16_t ui16_ph1_offset=0;
uint16_t ui16_ph2_offset=0;
uint16_t ui16_ph3_offset=0;
int16_t i16_ph1_current=0;

int16_t i16_ph2_current=0;
int16_t i16_ph2_current_filter=0;
int16_t i16_ph3_current=0;
uint16_t i=0;
uint16_t j=0;
uint16_t k=0;
uint16_t y=0;
uint8_t brake_flag=0;
volatile uint8_t ui8_overflow_flag=0;
uint8_t ui8_slowloop_counter=0;
volatile uint8_t ui8_adc_inj_flag=0;
volatile uint8_t ui8_adc_regular_flag=0;
uint8_t ui8_speedcase=0;
uint8_t ui8_speedfactor=0;
int8_t i8_direction= REVERSE; //for permanent reverse direction
int8_t i8_reverse_flag = 1; //for temporaribly reverse direction
uint8_t ui8_KV_detect_flag = 0; //for getting the KV of the motor after auto angle detect
uint16_t ui16_KV_detect_counter = 0; //for getting timing of the KV detect
int16_t ui32_KV = 0;


volatile uint8_t ui8_adc_offset_done_flag=0;
volatile uint8_t ui8_print_flag=0;
volatile uint8_t ui8_UART_flag=0;
volatile uint8_t ui8_Push_Assist_flag=0;
volatile uint8_t ui8_UART_TxCplt_flag=1;
volatile uint8_t ui8_PAS_flag=0;
volatile uint8_t ui8_SPEED_flag=0;
volatile uint8_t ui8_SPEED_control_flag=0;
volatile uint8_t ui8_BC_limit_flag=0;  //flag for Battery current limitation
volatile uint8_t ui8_6step_flag=0;
uint32_t uint32_PAS_counter= PAS_TIMEOUT+1;
uint32_t uint32_PAS_HIGH_counter= 0;
uint32_t uint32_PAS_HIGH_accumulated= 32000;
uint32_t uint32_PAS_fraction= 100;
uint32_t uint32_SPEED_counter=32000;
uint32_t uint32_SPEEDx100_cumulated=0;
uint32_t uint32_PAS=32000;

q31_t q31_rotorposition_PLL = 0;
q31_t q31_angle_per_tic = 0;

uint8_t ui8_UART_Counter=0;
int8_t i8_recent_rotor_direction=1;
int16_t i16_hall_order=1;
uint16_t ui16_erps=0;

uint32_t uint32_torque_cumulated=0;
uint32_t uint32_PAS_cumulated=32000;
uint16_t uint16_mapped_throttle=0;
uint16_t uint16_mapped_PAS=0;
uint16_t uint16_mapped_BRAKE=0;
uint16_t uint16_half_rotation_counter=0;
uint16_t uint16_full_rotation_counter=0;
int32_t int32_temp_current_target=0;
q31_t q31_PLL_error=0;
q31_t q31_t_Battery_Current_accumulated=0;

q31_t q31_rotorposition_absolute;
q31_t q31_rotorposition_hall;
q31_t q31_rotorposition_motor_specific = SPEC_ANGLE;
q31_t q31_u_d_temp=0;
q31_t q31_u_q_temp=0;
int16_t i16_sinus=0;
int16_t i16_cosinus=0;
char buffer[100];
char char_dyn_adc_state_old=1;
const uint8_t assist_factor[10]={0, 51, 102, 153, 204, 255, 255, 255, 255, 255};
const uint8_t assist_profile[2][6]= {	{0,10,20,30,45,48},
										{64,64,128,200,255,0}};

uint16_t switchtime[3];
volatile uint16_t adcData[8]; //Buffer for ADC1 Input
q31_t tic_array[6];

//Rotor angle scaled from degree to q31 for arm_math. -180Â°-->-2^31, 0Â°-->0, +180Â°-->+2^31
const q31_t deg_30 = 357913941;

q31_t Hall_13 = 0;
q31_t Hall_32 = 0;
q31_t Hall_26 = 0;
q31_t Hall_64 = 0;
q31_t Hall_51 = 0;
q31_t Hall_45 = 0;

const q31_t tics_lower_limit = WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*SPEEDLIMIT*10); //tics=wheelcirc*timerfrequency/(no. of hallevents per rev*gear-ratio*speedlimit)*3600/1000000
const q31_t tics_higher_limit = WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*(SPEEDLIMIT+2)*10);
uint32_t uint32_tics_filtered=1000000;

uint16_t VirtAddVarTab[NB_OF_VAR] = { 	EEPROM_POS_HALL_ORDER,
		EEPROM_POS_HALL_45,
		EEPROM_POS_HALL_51,
		EEPROM_POS_HALL_13,
		EEPROM_POS_HALL_32,
		EEPROM_POS_HALL_26,
		EEPROM_POS_HALL_64
	};

enum state {Stop, SixStep, Regen, Running, BatteryCurrentLimit, Interpolation, PLL, IdleRun};
enum state SystemState;

#define iabs(x) (((x) >= 0)?(x):-(x))
#define sign(x) (((x) >= 0)?(1):(-1))


//variables for display communication
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
KINGMETER_t KM;
#endif

//variables for display communication
#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
BAFANG_t BF;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
uint8_t ui8_main_LEV_Page_counter=0;
uint8_t ui8_additional_LEV_Page_counter=0;
uint8_t ui8_LEV_Page_to_send=1;
#endif



MotorState_t MS;
MotorParams_t MP;

//structs for PI_control
PI_control_t PI_iq;
PI_control_t PI_id;
PI_control_t PI_speed;


int16_t battery_percent_fromcapacity = 50; 			//Calculation of used watthours not implemented yet
int16_t wheel_time = 1000;							//duration of one wheel rotation for speed calculation
int16_t current_display;							//pepared battery current for display

int16_t power;										//recent power output

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void TIMER_Init(void);
static void ADC_Init(void);
int16_t T_NTC(uint16_t ADC);
                                

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
void kingmeter_update(void);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
void bafang_update(void);
#endif

static void dyn_adc_state(q31_t angle);
static void set_inj_channel(char state);
void get_standstill_position();
q31_t speed_PLL (q31_t ist, q31_t soll, uint8_t speedadapt);
int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
int32_t speed_to_tics (uint8_t speed);
int8_t tics_to_speed (uint32_t tics);
int16_t internal_tics_to_speedx100 (uint32_t tics);
int16_t external_tics_to_speedx100 (uint32_t tics);





/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */



  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();

  //initialize MS struct.
  MS.hall_angle_detect_flag=1;
  MS.Speed=128000;
  MS.assist_level=1;
  MS.regen_level=7;
	MS.i_q_setpoint = 0;
	MS.i_d_setpoint = 0;
	MS.angle_est=SPEED_PLL;


  MP.pulses_per_revolution = PULSES_PER_REVOLUTION;
  MP.wheel_cirumference = WHEEL_CIRCUMFERENCE;
  MP.speedLimit=SPEEDLIMIT;


  //init PI structs
  PI_id.gain_i=I_FACTOR_I_D;
  PI_id.gain_p=P_FACTOR_I_D;
  PI_id.setpoint = 0;
  PI_id.limit_output = _U_MAX;
  PI_id.max_step=5000;
  PI_id.shift=10;
  PI_id.limit_i=1800;

  PI_iq.gain_i=I_FACTOR_I_Q;
  PI_iq.gain_p=P_FACTOR_I_Q;
  PI_iq.setpoint = 0;
  PI_iq.limit_output = _U_MAX;
  PI_iq.max_step=5000;
  PI_iq.shift=10;
  PI_iq.limit_i=_U_MAX;

#ifdef SPEEDTHROTTLE

  PI_speed.gain_i=I_FACTOR_SPEED;
  PI_speed.gain_p=P_FACTOR_SPEED;
  PI_speed.setpoint = 0;
  PI_speed.limit_output = PH_CURRENT_MAX;
  PI_speed.max_step=50;
  PI_speed.shift=5;
  PI_speed.limit_i=PH_CURRENT_MAX;

#endif

  //Virtual EEPROM init
  HAL_FLASH_Unlock();
  EE_Init();
  HAL_FLASH_Lock();

  ADC_Init();

  /* USER CODE BEGIN 2 */
  TIMER_Init();

    
//PWM Mode 1: Interrupt at counting down.

    //TIM1->BDTR |= 1L<<15;
   // TIM1->BDTR &= ~(1L<<15); //reset MOE (Main Output Enable) bit to disable PWM output
    // Start Timer 2
    // HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    //HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    //HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);

       // Start Timer 3


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
       KingMeter_Init (&KM);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
       Bafang_Init (&BF);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
       kunteng_init();
       check_message(&MS, &MP);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
     //  ebics_init();
#endif


    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;



    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

    HAL_Delay(200); //wait for stable conditions

    for(i=0;i<32;i++){
    	while(!ui8_adc_regular_flag){}
    	temp1+=adcData[2];
    	temp2+=adcData[3];
    	temp3+=adcData[4];
    	ui8_adc_regular_flag=0;

    }
    ui16_ph1_offset=temp1>>5;
    ui16_ph2_offset=temp2>>5;
    ui16_ph3_offset=temp3>>5;

#ifdef DISABLE_DYNAMIC_ADC // set  injected channel with offsets
	 ADC1->JSQR=0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
	 ADC1->JOFR1 = ui16_ph1_offset;
	 ADC2->JSQR=0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
	 ADC2->JOFR1 = ui16_ph2_offset;
#endif

   	ui8_adc_offset_done_flag=1;

#if defined (ADC_BRAKE)

  	while ((adcData[5]>THROTTLE_OFFSET)&&(adcData[1]>(THROTTLE_MAX-THROTTLE_OFFSET))){HAL_Delay(200);
   	   	   			y++;
   	   	   			if(y==35) autodetect();
   	   	   			}

#endif

//run autodect, whenn brake is pulled an throttle is pulled for 10 at startup
#ifndef NCTE

  	while ((!HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin))&&(adcData[1]>(THROTTLE_OFFSET+20))){

  				HAL_Delay(200);
  	   			y++;
  	   			if(y==35) autodetect();
  	   			}
#else
  	ui32_throttle_cumulated=THROTTLE_OFFSET<<4;
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
   	printf_("phase current offsets:  %d, %d, %d \n ", ui16_ph1_offset, ui16_ph2_offset, ui16_ph3_offset);
#if (AUTODETECT == 1)
   	if(adcData[0]>VOLTAGE_MIN) autodetect();
   	else printf_("Battery voltage too low!:  %d,\n ",adcData[0]);
#endif

#endif

#ifdef NCTE
   	while(adcData[1]<THROTTLE_OFFSET)
#else
   	while(adcData[1]>THROTTLE_OFFSET)
#endif
   	  	{
   	  	//do nothing (For Safety at switching on)
   	  	}

#if (DISPLAY_TYPE != DISPLAY_TYPE_DEBUG || !AUTODETECT)
   	EE_ReadVariable(EEPROM_POS_HALL_ORDER, &i16_hall_order);
   	   	printf_("Hall_Order: %d \n",i16_hall_order);
   	   	// set varaiables to value from emulated EEPROM only if valid
   	   	if(i16_hall_order!=0xFFFF) {
   	   		int16_t temp;

   	   		EE_ReadVariable(EEPROM_POS_HALL_45, &temp);
   	   		Hall_45 = temp<<16;
   	   		printf_("Hall_45: %d \n",	(int16_t) (((Hall_45 >> 23) * 180) >> 8));

   	   		EE_ReadVariable(EEPROM_POS_HALL_51, &temp);
   	   		Hall_51 = temp<<16;
   	   		printf_("Hall_51: %d \n",	(int16_t) (((Hall_51 >> 23) * 180) >> 8));

   	   		EE_ReadVariable(EEPROM_POS_HALL_13, &temp);
   	   		Hall_13 = temp<<16;
   	   		printf_("Hall_13: %d \n",	(int16_t) (((Hall_13 >> 23) * 180) >> 8));

   	   		EE_ReadVariable(EEPROM_POS_HALL_32, &temp);
   	   		Hall_32 = temp<<16;
   	   		printf_("Hall_32: %d \n",	(int16_t) (((Hall_32 >> 23) * 180) >> 8));

   	   		EE_ReadVariable(EEPROM_POS_HALL_26, &temp);
   	   		Hall_26 = temp<<16;
   	   		printf_("Hall_26: %d \n",	(int16_t) (((Hall_26 >> 23) * 180) >> 8));

   	   		EE_ReadVariable(EEPROM_POS_HALL_64, &temp);
   	  		Hall_64 = temp<<16;
   	  		printf_("Hall_64: %d \n",	(int16_t) (((Hall_64 >> 23) * 180) >> 8));

   	  		EE_ReadVariable(EEPROM_POS_KV, &ui32_KV);
   	  		if(!ui32_KV)ui32_KV=111;
   	  		printf_("KV: %d \n",ui32_KV	);

   	   	}

#endif


 // set absolute position to corresponding hall pattern.

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Lishui FOC v1.0 \n ");

#endif


    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

	get_standstill_position();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	 /* if(PI_flag){
	  runPIcontrol();
	  PI_flag=0;
	  }*/
	  //display message processing
	  if(ui8_UART_flag){
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
	  kingmeter_update();
#endif


#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
	  bafang_update();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
	  check_message(&MS, &MP);
	  if(MS.assist_level==6)ui8_Push_Assist_flag=1;
	  else ui8_Push_Assist_flag=0;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
	//  process_ant_page(&MS, &MP);
#endif

	  ui8_UART_flag=0;
	  }


	  //process regualr ADC
	  if(ui8_adc_regular_flag){
		ui32_throttle_cumulated -= ui32_throttle_cumulated>>4;
	#ifdef TQONAD1
		ui32_throttle_cumulated += adcData[6]; //get value from AD1 PB1
	#else
		ui32_throttle_cumulated += adcData[1]; //get value from SP
	#endif
		ui32_brake_adc_cumulated -= ui32_brake_adc_cumulated>>4;
		ui32_brake_adc_cumulated+=adcData[5];//get value for analog brake from AD2 = PB0
		ui16_brake_adc=ui32_brake_adc_cumulated>>4;
		ui16_throttle = ui32_throttle_cumulated>>4;

		ui8_adc_regular_flag=0;

	  }

	  //PAS signal processing
	  if(ui8_PAS_flag){
		  if(uint32_PAS_counter>100){ //debounce
		  uint32_PAS_cumulated -= uint32_PAS_cumulated>>2;
		  uint32_PAS_cumulated += uint32_PAS_counter;
		  uint32_PAS = uint32_PAS_cumulated>>2;

		  uint32_PAS_HIGH_accumulated-=uint32_PAS_HIGH_accumulated>>2;
		  uint32_PAS_HIGH_accumulated+=uint32_PAS_HIGH_counter;

		  uint32_PAS_fraction=(uint32_PAS_HIGH_accumulated>>2)*100/uint32_PAS;
		  uint32_PAS_HIGH_counter=0;
		  uint32_PAS_counter =0;
		  ui8_PAS_flag=0;
		  //read in and sum up torque-signal within one crank revolution (for sempu sensor 32 PAS pulses/revolution, 2^5=32)
		  uint32_torque_cumulated -= uint32_torque_cumulated>>5;
#ifdef NCTE
		  if(ui16_throttle<THROTTLE_OFFSET)uint32_torque_cumulated += (THROTTLE_OFFSET-ui16_throttle);
#else
		  if(ui16_throttle>THROTTLE_OFFSET)uint32_torque_cumulated += (ui16_throttle-THROTTLE_OFFSET);
#endif
		  }
	  }

	  //SPEED signal processing
#if (SPEEDSOURCE == INTERNAL)
			  MS.Speed = uint32_tics_filtered>>3;
#else

	  if(ui8_SPEED_flag){

		  if(uint32_SPEED_counter>200){ //debounce
			  MS.Speed = uint32_SPEED_counter;
			  //HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			  uint32_SPEED_counter =0;
			  ui8_SPEED_flag=0;
			  uint32_SPEEDx100_cumulated -=uint32_SPEEDx100_cumulated>>SPEEDFILTER;
			  uint32_SPEEDx100_cumulated +=external_tics_to_speedx100(MS.Speed);
		  }
	  }
#endif
	  if(ui8_SPEED_control_flag){
#if (SPEEDSOURCE == INTERNAL)
		uint32_SPEEDx100_cumulated -=uint32_SPEEDx100_cumulated>>SPEEDFILTER;
		uint32_SPEEDx100_cumulated +=internal_tics_to_speedx100(uint32_tics_filtered>>3);
#endif
		ui16_erps=500000/((uint32_tics_filtered>>3)*6);
		ui8_SPEED_control_flag=0;
	  }

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && defined(FAST_LOOP_LOG))
		if(ui8_debug_state==3 && ui8_UART_TxCplt_flag){
	        sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n", e_log[k][0], e_log[k][1], e_log[k][2],e_log[k][3],e_log[k][4],e_log[k][5]); //>>24
			i=0;
			while (buffer[i] != '\0')
			{i++;}
			ui8_UART_TxCplt_flag=0;
			HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&buffer, i);
			k++;
			if (k>299){
				k=0;
				ui8_debug_state=0;
				//Obs_flag=0;
			}
		}
#endif

		//--------------------------------------------------------------------------------------------------------------------------------------------------

			  //current target calculation
			//highest priority: regen by brake lever


#ifdef ADC_BRAKE
		uint16_mapped_BRAKE = map(ui16_brake_adc, THROTTLE_OFFSET , THROTTLE_MAX, 0, REGEN_CURRENT);


		if(uint16_mapped_BRAKE>0) brake_flag=1;
		else brake_flag=0;


		if(brake_flag){

				//if(!HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)){
					//if(tics_to_speed(uint32_tics_filtered>>3)>6)int32_current_target=-REGEN_CURRENT; //only apply regen, if motor is turning fast enough
				if(tics_to_speed(uint32_tics_filtered>>3)>6)int32_temp_current_target=uint16_mapped_BRAKE;
				else int32_temp_current_target=0;


#else
		if(HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)) brake_flag=0;
		else brake_flag=1;
				if(brake_flag){

						if(tics_to_speed(uint32_tics_filtered>>3)>6){
							int32_temp_current_target=REGEN_CURRENT; //only apply regen, if motor is turning fast enough
							}
						else int32_temp_current_target=0;

#endif
	  			int32_temp_current_target= -map(MS.Voltage*CAL_V,BATTERYVOLTAGE_MAX-1000,BATTERYVOLTAGE_MAX,int32_temp_current_target,0);
				}

				//next priority: undervoltage protection
				else if(MS.Voltage<VOLTAGE_MIN)int32_temp_current_target=0;
				//next priority: push assist
				else if(ui8_Push_Assist_flag)int32_temp_current_target=PUSHASSIST_CURRENT;
				// last priority normal ride conditiones
				else {

		#ifdef TS_MODE //torque-sensor mode
					//calculate current target form torque, cadence and assist level
					int32_temp_current_target = (TS_COEF*(int16_t)(MS.assist_level)* (uint32_torque_cumulated>>5)/uint32_PAS)>>8; //>>5 aus Mittelung über eine Kurbelumdrehung, >>8 aus KM5S-Protokoll Assistlevel 0..255

					//limit currest target to max value
					if(int32_temp_current_target>PH_CURRENT_MAX) int32_temp_current_target = PH_CURRENT_MAX;
					//set target to zero, if pedals are not turning
					if(uint32_PAS_counter > PAS_TIMEOUT){
						int32_temp_current_target = 0;
						if(uint32_torque_cumulated>0)uint32_torque_cumulated--; //ramp down cumulated torque value
					}



		#else		// torque-simulation mode with throttle override

		#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
			  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(assist_factor[MS.assist_level]))>>8, 0); // level in range 0...5
		#endif

		#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
			  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(MS.assist_level))/5, 0); // level in range 0...5
		#endif

		#if (DISPLAY_TYPE == DISPLAY_TYPE_KINGMETER_618U)
			  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(MS.assist_level-1))>>2, 0); // level in range 1...5
		#endif

		#if (DISPLAY_TYPE == DISPLAY_TYPE_KINGMETER_901U)
			  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, ((PH_CURRENT_MAX*(int32_t)(MS.assist_level)))>>8, 0); // level in range 0...255
		#endif

		#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
			 uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, PH_CURRENT_MAX, 0); // Full amps in debug mode
		#endif



		#ifdef DIRDET
			  if (uint32_PAS_counter< PAS_TIMEOUT){
				  if ((uint32_PAS_fraction < FRAC_LOW ||uint32_PAS_fraction > FRAC_HIGH)){
					  uint16_mapped_PAS= 0;//pedals are turning backwards, stop motor
				  }
			  }
			  else uint32_PAS_HIGH_accumulated=uint32_PAS_cumulated;
		#endif //end direction detection


		#endif //end if else TQ sensor mode

#ifdef INDIVIDUAL_MODES

			  uint16_mapped_PAS = (uint16_mapped_PAS * ui8_speedfactor)>>8;

#endif

#ifdef THROTTLE_OVERRIDE


#ifdef NCTE
			  // read in throttle for throttle override
			  uint16_mapped_throttle = map(ui16_throttle, THROTTLE_MAX, THROTTLE_OFFSET,PH_CURRENT_MAX,0);


#else //else NTCE
			  // read in throttle for throttle override
			  uint16_mapped_throttle = map(ui16_throttle, THROTTLE_OFFSET, THROTTLE_MAX, 0,PH_CURRENT_MAX);

#endif //end NTCE

#ifndef TS_MODE //normal PAS Mode

			    if (uint32_PAS_counter < PAS_TIMEOUT) int32_temp_current_target = uint16_mapped_PAS;		//set current target in torque-simulation-mode, if pedals are turning
				  else  {
					  int32_temp_current_target= 0;//pedals are not turning, stop motor
					  uint32_PAS_cumulated=32000;
					  uint32_PAS=32000;
				  }

#endif		// end #ifndef TS_MODE
			    //check for throttle override
				if(uint16_mapped_throttle>int32_temp_current_target){

#ifdef SPEEDTHROTTLE


					uint16_mapped_throttle = uint16_mapped_throttle*SPEEDLIMIT/PH_CURRENT_MAX;//throttle override: calulate speed target from thottle




					  PI_speed.setpoint = uint16_mapped_throttle*100;
					  PI_speed.recent_value = internal_tics_to_speedx100(uint32_tics_filtered>>3);
					 if( PI_speed.setpoint)SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
					if (internal_tics_to_speedx100(uint32_tics_filtered>>3)<300){//control current slower than 3 km/h
						PI_speed.limit_i=100;
						PI_speed.limit_output=100;
						int32_temp_current_target = PI_control(&PI_speed);

						if(int32_temp_current_target>100)int32_temp_current_target=100;
						if(int32_temp_current_target*i8_direction*i8_reverse_flag<0){
							int32_temp_current_target=0;
						}

					}
					else{


						if(ui8_SPEED_control_flag){//update current target only, if new hall event was detected
							PI_speed.limit_i=PH_CURRENT_MAX;
							PI_speed.limit_output=PH_CURRENT_MAX;
							int32_temp_current_target = PI_control(&PI_speed);
							ui8_SPEED_control_flag=0;
							}
						if(int32_temp_current_target*i8_direction*i8_reverse_flag<0)int32_temp_current_target=0;

						}



#else // end speedthrottle
					int32_temp_current_target=uint16_mapped_throttle;
#endif  //end speedthrottle

				  } //end else of throttle override

#endif //end throttle override

				} //end else for normal riding
				  //ramp down setpoint at speed limit
#ifdef LEGALFLAG
			if(!brake_flag){ //only ramp down if no regen active
				if(uint32_PAS_counter<PAS_TIMEOUT){
					int32_temp_current_target=map(uint32_SPEEDx100_cumulated>>SPEEDFILTER, MP.speedLimit*100,(MP.speedLimit+2)*100,int32_temp_current_target,0);
					}
				else{ //limit to 6km/h if pedals are not turning
					int32_temp_current_target=map(uint32_SPEEDx100_cumulated>>SPEEDFILTER, 500,700,int32_temp_current_target,0);
					}
				}
//			else int32_temp_current_target=int32_temp_current_target;
#else //legalflag
				MS.i_q_setpoint=int32_temp_current_target;
#endif //legalflag
				MS.i_q_setpoint=map(MS.Temperature, 120,130,int32_temp_current_target,0); //ramp down power with temperature to avoid overheating the motor
				//auto KV detect
			  if(ui8_KV_detect_flag){
				  MS.i_q_setpoint=ui8_KV_detect_flag;
				  if(ui16_KV_detect_counter>32){
					  ui8_KV_detect_flag++;
					  ui16_KV_detect_counter=0;
					  if(MS.u_abs>1900){
						  ui8_KV_detect_flag=0;
					 	  HAL_FLASH_Unlock();
					      EE_WriteVariable(EEPROM_POS_KV, (int16_t) ui32_KV);
					      HAL_FLASH_Lock();
					  }
				  }
				  ui32_KV -=ui32_KV>>4;
				  ui32_KV += ((uint32_SPEEDx100_cumulated*_T))/(MS.Voltage*MS.u_q);


			  }//end KV detect



//------------------------------------------------------------------------------------------------------------
				//enable PWM if power is wanted
	  if (MS.i_q_setpoint>0&&!READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){

		  uint16_half_rotation_counter=0;
		  uint16_full_rotation_counter=0;
		    TIM1->CCR1 = 1023; //set initial PWM values
		    TIM1->CCR2 = 1023;
		    TIM1->CCR3 = 1023;
		    SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
		    if(SystemState == Stop)speed_PLL(0,0,0);//reset integral part
		    else {
		    	PI_iq.integral_part = ((((uint32_SPEEDx100_cumulated*_T))/(MS.Voltage*ui32_KV))<<4)<<PI_iq.shift;
		    	PI_iq.out=PI_iq.integral_part;
		    }
		  __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
		  ui16_timertics=20000; //set interval between two hallevents to a large value
		  i8_recent_rotor_direction=i8_direction*i8_reverse_flag;
		  get_standstill_position();
	  }


//----------------------------------------------------------------------------------------------------------------------------------------------------------
	  //slow loop procedere @16Hz, for LEV standard every 4th loop run, send page,
	  if(ui32_tim3_counter>500){


		if(HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin)){

      	//HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
      	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
      	//HAL_GPIO_WritePin(BRAKE_LIGHT_GPIO_Port, BRAKE_LIGHT_Pin, GPIO_PIN_RESET);
		}
		else{
	      	//HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
	      	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	      	//HAL_GPIO_WritePin(BRAKE_LIGHT_GPIO_Port, BRAKE_LIGHT_Pin, GPIO_PIN_SET);
		}


		  if(ui8_KV_detect_flag){ui16_KV_detect_counter++;}
#if (R_TEMP_PULLUP)
		  MS.Temperature = T_NTC(adcData[6]); //Thank you Hendrik ;-)
#else
		  MS.Temperature=25;
#endif
		  MS.Voltage=adcData[0];
		  if(uint32_SPEED_counter>127999){
			  MS.Speed =128000;
#if (SPEEDSOURCE == EXTERNAL)
			  uint32_SPEEDx100_cumulated=0;
#endif
		  }

#ifdef INDIVIDUAL_MODES
		  // GET recent speedcase for assist profile
		  if (uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][1]))ui8_speedcase=0;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][1]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][2]))ui8_speedcase=1;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][2]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][3]))ui8_speedcase=2;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][3]) && uint32_tics_filtered>>3 > speed_to_tics(assist_profile[0][4]))ui8_speedcase=3;
		  else if (uint32_tics_filtered>>3 < speed_to_tics(assist_profile[0][4]))ui8_speedcase=4;

		  ui8_speedfactor = map(uint32_tics_filtered>>3,speed_to_tics(assist_profile[0][ui8_speedcase+1]),speed_to_tics(assist_profile[0][ui8_speedcase]),assist_profile[1][ui8_speedcase+1],assist_profile[1][ui8_speedcase]);


#endif
//check if rotor is turning

		  if((uint16_full_rotation_counter>7999||uint16_half_rotation_counter>7999)){
			  SystemState = Stop;
			  if(READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){
			  CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning
			  uint32_tics_filtered=1000000;
			  get_standstill_position();
			  }

		  }
		  else if(ui8_6step_flag) SystemState = SixStep;
		  else SystemState = Running;

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && !defined(FAST_LOOP_LOG))
		  //print values for debugging


		 sprintf_(buffer, "%d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", adcData[1],MS.Temperature, ui32_KV, MS.i_q_setpoint, uint32_PAS, int32_temp_current_target , MS.u_d,MS.u_q, SystemState);
		 // sprintf_(buffer, "%d, %d, %d, %d, %d, %d, %d\r\n",(uint16_t)adcData[0],(uint16_t)adcData[1],(uint16_t)adcData[2],(uint16_t)adcData[3],(uint16_t)(adcData[4]),(uint16_t)(adcData[5]),(uint16_t)(adcData[6])) ;
		 // sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n",tic_array[0],tic_array[1],tic_array[2],tic_array[3],tic_array[4],tic_array[5]) ;
		  i=0;
		  while (buffer[i] != '\0')
		  {i++;}
		 HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&buffer, i);


		  ui8_print_flag=0;

#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
		  ui8_slowloop_counter++;
		  if(ui8_slowloop_counter>3){
			  ui8_slowloop_counter = 0;

			  switch (ui8_main_LEV_Page_counter){
			  case 1: {
				  ui8_LEV_Page_to_send = 1;
			  	  }
			  	  break;
			  case 2: {
				  ui8_LEV_Page_to_send = 2;
			  	  }
			  	  break;
			  case 3: {
				  ui8_LEV_Page_to_send = 3;
			  	  }
			  	  break;
			  case 4: {
				  //to do, define other pages
				  ui8_LEV_Page_to_send = 4;
			  	  }
			  	  break;
			  }//end switch

			//  send_ant_page(ui8_LEV_Page_to_send, &MS, &MP);

			  ui8_main_LEV_Page_counter++;
			  if(ui8_main_LEV_Page_counter>4)ui8_main_LEV_Page_counter=1;
		  }

#endif
		  ui32_tim3_counter=0;
	  }// end of slow loop

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

#define RCC_PLLON_BIT_NUMBER      RCC_CR_PLLON_Pos
#define RCC_CR_PLLON_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_PLLON_BIT_NUMBER * 4U)))
#define __RCC_PLL_ENABLE()          (*(__IO uint32_t *) RCC_CR_PLLON_BB = ENABLE)

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  /** System starts up on HSI with PLL disabled.
	 * switch to HSI, PLL *16 / 2 = 64 MHz (max. for the STM32F103 with HSI only)
   * ADC Clock PCLK2/6 = 10.66 MHz (design says 14 MHz max, /4 would be 16 MHz)
   * APB2 Prescaler 1 = HCLK = 64 Mhz (PCLK2)
   * APB1 Clock = HCLK / 2 (Max. 36 MHz by design -> 32 MHz) (PCLK1)
   * AHB Clock = SYSCLK = 64 MHz
	 */
  RCC->CFGR = RCC_CFGR_PLLMULL16 | RCC_CFGR_ADCPRE_DIV6 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_HPRE_DIV1;
  __RCC_PLL_ENABLE();
  while(!(RCC->CR & RCC_CR_PLLRDY)) {
    /* wait for PLL to be locked. Library code has a timeout check here which ends up in the error handler that implements an
    endless loop, so we can also just stay here. */
  }
  /* 2 wait states need to be configured for flash memory, because we will run at > 48 MHz */
  /* Flash latency init is 0, so no reset neccessary */
  FLASH->ACR |= FLASH_ACR_LATENCY_1;  /* register naming ambiguous here: This sets Bit 1 which is 2 wait states */
  /* switch clock source to PLL */
  RCC->CFGR |= RCC_CFGR_SW_PLL;

  /* System now running with 64 MHz */
  SystemCoreClock = 64000000;


  /**Configure the Systick 
  */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_InitTick (TICK_INT_PRIORITY);
}


/* ADC init function:
  Configures both ADCs and the corresponding DMA1CH1. Enables all peripherals as well. */
static void ADC_Init(void)
{
  /* Enable ADC clocks */
  RCC->APB2ENR |= (1 << RCC_APB2ENR_ADC1EN_Pos) | (1 << RCC_APB2ENR_ADC2EN_Pos);
  RCC->AHBENR |= (1 << RCC_AHBENR_DMA1EN_Pos);
  /* A 2-clock cycle delay should be added here before ADC registers can be accessed. (1 clock cycle delay
    by write to RCC register above already done) */
  __NOP();  

  /* No analog watchdog (ToDo!)
   Dual mode: Simultaneous for regular + injected, that means a trigger to ADC1 starts ADC2 as well. Matching sampling-times has to be specified
   for each configured channel pair that will be converted at the same time. Also, DMA has to be enabled; external triggers have to be enabled for
   both ADC1 + 2, but 2 should not be configured to the same trigger source.
   ToDo: If there is any use, ADC2 can be used for some regular conversions as well.
   No discontinuous mode
   no automatic injected group
   Scan mode: All regular channels in a group are converted
   Interrupt for injected channels (ToDo: Configure)
   No interrupt on EOC (DMA)
   Enable temperature/Vref (for future temperature sensor readout)
   Enable external trigger for regular channels on Timer 3 TRGO (ToDo: Maybe use continuous mode?)
   Enable external trigger for injected channels on Timer 1 CC4
   DataAlign right
   No continuous mode: Conversions need to be triggered
   ADC on
   */
  ADC1->CR1 = (1 << ADC_CR1_DUALMOD_Pos) | (1 << ADC_CR1_SCAN_Pos) | (1 << ADC_CR1_JEOSIE_Pos);
  ADC1->CR2 = (1 << ADC_CR2_TSVREFE_Pos) | (1 << ADC_CR2_EXTTRIG_Pos) | (4 << ADC_CR2_EXTSEL_Pos) | 
              (1 << ADC_CR2_JEXTTRIG_Pos) | (1 << ADC_CR2_JEXTSEL_Pos) | (0 << ADC_CR2_ALIGN_Pos) |
              (1 << ADC_CR2_DMA_Pos) | (1 << ADC_CR2_ADON_Pos);

  /* ADC2: Subset of ADC1 configuration:
   Scan mode, Software Start Trigger, 1 injected channel.
   Interrupt not necessary, as ADC1 interrupt will notify about the data.
   Trigger injected conversion on external trigger SWSTART
   */
  ADC2->CR1 = (1 << ADC_CR1_SCAN_Pos) | (0 << ADC_CR1_JEOSIE_Pos);
  ADC2->CR2 = (1 << ADC_CR2_JEXTTRIG_Pos) | (7 << ADC_CR2_JEXTSEL_Pos) | (1 << ADC_CR2_ADON_Pos);


  /* Configure Sample time registers:
   everything with 1.5 cycles = 140 ns
   Temperature sensor with 239.5 cycles = 22.4 µs
   ToDo: Maybe slightly higher sampling times for channels 7/3/8/9 ? I don't know about the external capacity on those pins.
  */
  ADC1->SMPR1 = (111 << ADC_SMPR1_SMP16_Pos);
  ADC1->SMPR2 = 0;

  ADC2->SMPR1 = 0;
  ADC2->SMPR2 = 0;

    /* ADC Channel definition:
    7: Battery Voltage
    3: SP Throttle
    4: Phase current 1
    5: Phase current 2
    6: Phase current 3
    8: AD2 / Temperature
    9: Temperature or Torque on PhoebeLiu @ aliexpress
    16: STM32 temperature sensor - must be sampled with 17.1 µs and might have very high chip-to-chip offset variations (datasheet says 45K)
  */

 /**Configure Regular Channel
   * L = 7 for 8 conversions.
  */
  ADC1->SQR3 = (7 << ADC_SQR3_SQ1_Pos) |
              (3 << ADC_SQR3_SQ2_Pos) |
              (4 << ADC_SQR3_SQ3_Pos) |
              (5 << ADC_SQR3_SQ4_Pos) |
              (6 << ADC_SQR3_SQ5_Pos) |
              (8 << ADC_SQR3_SQ6_Pos);
  ADC1->SQR2 = (9 << ADC_SQR2_SQ7_Pos) |
              (16 << ADC_SQR2_SQ8_Pos);
  ADC1->SQR1 = (7 << ADC_SQR1_L_Pos);

  /* Configure ADC1 injected Channel 4 (Phase 1 current):
     - Sequence Length = 1 (JL=0)
     - JSQ4 = 4 (first/only sequence converted )
     Injected channels will be dynamically selected by the calculated PWM duty cycle later.
  */
  ADC1->JSQR = (4 << ADC_JSQR_JSQ4_Pos);
  /* Set offset to be substracted */
  ADC1->JOFR1 = ui16_ph1_offset;

  /* Configure ADC2 injected channel 5 (Phase 2 current):
     - Sequence length = 1 (JL=0)
     - JSQ4 = 4 (first/only sequence converted) 
     Injected channels will be dynamically selected by the calculated PWM duty cycle later.
  */
  ADC2->JSQR = (5 << ADC_JSQR_JSQ4_Pos);
  /* Set offset to be substracted */
  ADC2->JOFR1 = ui16_ph1_offset;

  ADC1->CR2 |= (1 << ADC_CR2_RSTCAL_Pos);
  ADC2->CR2 |= (1 << ADC_CR2_RSTCAL_Pos);
  HAL_Delay(1);

  /* perform ADC calibration */
  ADC1->CR2 |= (1 << ADC_CR2_CAL_Pos);
  ADC2->CR2 |= (1 << ADC_CR2_CAL_Pos);
  HAL_Delay(1);

  /* Configure DMA for ADC1 (DMA1, Channel 1)
     Transfer 16 bit Periph to 16 bit memory
     Memory circular buffer
     interrupt on transfer completion*/
  DMA1_Channel1->CCR = (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) | (1 << DMA_CCR_MINC_Pos) |
                      (1 << DMA_CCR_CIRC_Pos) | (1 << DMA_CCR_TCIE_Pos);
  DMA1_Channel1->CNDTR = 8;
  DMA1_Channel1->CPAR = (uint32_t)&(ADC1->DR);
  DMA1_Channel1->CMAR = (uint32_t)adcData;

  /* Enable ADC DMA Channel*/
  DMA1_Channel1->CCR |= (1 << DMA_CCR_EN_Pos);
  
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

/* Configures Timer1-3:
  Timer 1 - Motor PWM, 64 MHz base clock, Up/Down counting PWM at around 15.8 kHz
  Timer 2 - HALL sensor readings via input capture interrupt, internal speed sensor generation
  Timer 3 - regular ADC trigger, slow loop timing control
  */
static void TIMER_Init(void)
{
  /* Enable clock to timer modules */
  RCC->APB2ENR |= (1 << RCC_APB2ENR_TIM1EN_Pos);
  RCC->APB1ENR |= (1 << RCC_APB1ENR_TIM2EN_Pos) | (1 << RCC_APB1ENR_TIM3EN_Pos);

  /* Center aligned mode 1: OC Interrupt when downcounting */
  TIM1->CR1 = (1 << TIM_CR1_CMS_Pos);
  /* ToDo: CCPC/CCUS
    Output Idle state = 0 for P and =1 for N channels
    Output Idle state = 1 for P of channel 4 (ADC trigger)
    Master Mode: OC4REF triggers slaves (TRGO) */
  TIM1->CR2 = (1 << TIM_CR2_OIS1N_Pos) | (1 << TIM_CR2_OIS2N_Pos) | (1 << TIM_CR2_OIS3N_Pos) |
            (1 << TIM_CR2_OIS4_Pos) | (7 << TIM_CR2_MMS_Pos);
  /*
  Preload for compare register enabled (-> update events could come at top and bottom I guess, but shouldn't matter too much)
  PWM Mode 1: Output active below compare register (Duty-cycle 100% at compare-register=period)

  */
  TIM1->CCMR1 = (1 << TIM_CCMR1_OC1PE_Pos) | (6 << TIM_CCMR1_OC1M_Pos) |
              (1 << TIM_CCMR1_OC2PE_Pos) | (6 << TIM_CCMR1_OC2M_Pos);
  TIM1->CCMR2 = (1 << TIM_CCMR2_OC3PE_Pos) | (6 << TIM_CCMR2_OC3M_Pos) |
              (1 << TIM_CCMR2_OC4PE_Pos) | (6 << TIM_CCMR2_OC4M_Pos);

  /* Output Compares active high (P-Ch); active low (N-Ch)
    Interrupt/ADC trigger channel (P) active low
  */
  TIM1->CCER = (1 << TIM_CCER_CC1E_Pos) | (1 << TIM_CCER_CC1NE_Pos) | (1 << TIM_CCER_CC1NP_Pos) |
              (1 << TIM_CCER_CC2E_Pos) | (1 << TIM_CCER_CC2NE_Pos) | (1 << TIM_CCER_CC2NP_Pos) |
              (1 << TIM_CCER_CC3E_Pos) | (1 << TIM_CCER_CC3NE_Pos) | (1 << TIM_CCER_CC3NP_Pos) |
              (1 << TIM_CCER_CC4E_Pos) | (1 << TIM_CCER_CC4P_Pos);

  /* set timer period. Resulting PWM frequency is 64 MHz / (ARR + 1) / 2 (center aligned/up&down counting).
    PWM dutycycle 0% (Compare = 0) to 100% (Compare >= _T = 2028) */
  TIM1->ARR = _T;

  /* Set initial PWM duty cycles.
   */
  TIM1->CCR1 = _T/2;
  TIM1->CCR2 = _T/2;
  TIM1->CCR3 = _T/2;
  TIM1->CCR4 = TRIGGER_DEFAULT; //ADC sampling just before timer overflow (just before middle of PWM-Cycle)

  /* Deadtime 32 (timer cycles) e.g. 500ns, Compare outputs disable in idle (e.g. GPIO takes over)
   ToDo: There is external gate drivers with deadtime generation, so these deadtimes add up.
   (IRS2003: about 500ns, turn on delay 680-820ns; turn off delay 150-220ns)
   Mosfets have about 30-60ns delay turn on and 30-80ns turn off, so a (total) deadtime of 100ns should be quite safe.
   Deadtime should be considered when defining sample points for the phase current measurements. */
  TIM1->BDTR = (32 << TIM_BDTR_DTG_Pos);

  /* There are no interrupts of Timer 1 in use, so leave them disabled */


/* Timer 2 */
  TIM2->CR1 = 0;
  /* TI1 Input XOR of CH1/CH2/CH3*/
  TIM2->CR2 = (1 << TIM_CR2_TI1S_Pos);
  /* Slave mode: Reset, Trigger by TI1 Edge detector */
  TIM2->SMCR = (4 << TIM_SMCR_TS_Pos) | (4 << TIM_SMCR_SMS_Pos);
  /* Interrupt on trigger (which is any of the Hall sensor state changes) */
  TIM2->DIER = (1 << TIM_DIER_TIE_Pos);
  /* ToDo: Original filter sets the filter register to 8 which does sampling at
     fdts/8 with a filter length of 6, effectively creating a filter delay of
     1,5 µs*/
  /* Enable input capture on Channel 1 (=TRC) with 8 cycle filter */
  /* ToDo: Original code sets channel 2 and 3 as well, which we don't need */
  TIM2->CCMR1 = (3 << TIM_CCMR1_CC1S_Pos) | (7 << TIM_CCMR1_IC1F_Pos);
  TIM2->CCER = (1 << TIM_CCER_CC1E_Pos);
  /* ToDo: ST code uses dynamic prescaler */
  TIM2->PSC = 128;
  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);


/* Timer 3:Upcounting (0..ARR) */

  TIM3->CR1 = 0;
  /* TRGO on CNT = OC1. (Triggers ADC1 regular conversion)
   Does not need to be enabled further, OC1 = 0, so basically trigger on overflow. */
  TIM3->CR2 = (3 << TIM_CR2_MMS_Pos);
  TIM3->ARR = 7813;
  TIM3->DIER = (1 << TIM_DIER_UIE_Pos);

  HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);


  /* Enable Timers */
  TIM1->CR1 |= (1 << TIM_CR1_CEN_Pos);
  TIM1->BDTR |= (1 << TIM_BDTR_MOE_Pos);
  TIM2->CR1 |= (1 << TIM_CR1_CEN_Pos);
  TIM3->CR1 |= (1 << TIM_CR1_CEN_Pos);

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;

#if ((DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER) ||DISPLAY_TYPE==DISPLAY_TYPE_KUNTENG||DISPLAY_TYPE==DISPLAY_TYPE_EBiCS)
  huart1.Init.BaudRate = 9600;
#elif (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  huart1.Init.BaudRate = 1200;
#else
  huart1.Init.BaudRate = 56000;
#endif


  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();



  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = Throttle_Pin|Phase_Current1_Pin|Phase_Current_2_Pin|Phase_Current_3_Pin|GPIO_PIN_7; //128 for PA7 = AIN7
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin =  Temperature_Pin|GPIO_PIN_0|GPIO_PIN_1; //for ADC8+9
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Hall_1_Pin Hall_2_Pin Hall_3_Pin */
  GPIO_InitStruct.Pin = Hall_1_Pin|Hall_2_Pin|Hall_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LIGHT_Pin */
  GPIO_InitStruct.Pin = LIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LIGHT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BRAKE_LIGHT_Pin */
  GPIO_InitStruct.Pin = BRAKE_LIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BRAKE_LIGHT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Brake_Pin */
  GPIO_InitStruct.Pin = Brake_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Brake_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pins : Speed_EXTI5_Pin PAS_EXTI8_Pin */
  GPIO_InitStruct.Pin = Speed_EXTI5_Pin|PAS_EXTI8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


  /* EXTI interrupt init*/


  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* Timer 1 PWM Pins (FET drive) */
      GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

void timer3_elapsed(void)
{

#ifdef SPEED_PLL
		   if(!READ_BIT(TIM1->BDTR, TIM_BDTR_MOE))q31_rotorposition_PLL += (q31_angle_per_tic<<1);
#endif

		if(ui32_tim3_counter<32000)ui32_tim3_counter++;
		if (uint32_PAS_counter < PAS_TIMEOUT+1){
			  uint32_PAS_counter++;
			  if(HAL_GPIO_ReadPin(PAS_GPIO_Port, PAS_Pin))uint32_PAS_HIGH_counter++;
		}
		if (uint32_SPEED_counter<128000)uint32_SPEED_counter++;					//counter for external Speedsensor
		if(uint16_full_rotation_counter<8000)uint16_full_rotation_counter++;	//full rotation counter for motor standstill detection
		if(uint16_half_rotation_counter<8000)uint16_half_rotation_counter++;	//half rotation counter for motor standstill detection

}


/* Called from interrupt handler to signal the completion of the injected ADC measurements:
  Phase currents (of previously configured channels: 2 out of 3)*/
void phase_current_measurement_complete()
{
	//for oszi-check of used time in FOC procedere
	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  ui32_tim1_counter++;

	/*  else {
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  uint32_SPEED_counter=0;
	  }*/

	if(!ui8_adc_offset_done_flag)
	{
    /* ToDo: While the offsets are not calculated, we should not use any data.
       Actually, the main application does not run anyway - likely we also don't need this here at all? */
    i16_ph1_current = ADC1->JDR1;
    i16_ph2_current = ADC2->JDR1;

  	ui8_adc_inj_flag=1;
	}
	else{

#ifdef DISABLE_DYNAMIC_ADC

    i16_ph1_current = ADC1->JDR1;
    i16_ph2_current = ADC2->JDR1;


#else
	switch (MS.char_dyn_adc_state) //read in according to state
		{
		case 1: //Phase C at high dutycycles, read from A+B directly
			{
				temp1=(q31_t)ADC1->JDR1;
				i16_ph1_current = temp1 ;

				temp2=(q31_t)ADC2->JDR1;
				i16_ph2_current = temp2;
			}
			break;
		case 2: //Phase A at high dutycycles, read from B+C (A = -B -C)
			{

				temp2=(q31_t)ADC2->JDR1;
				i16_ph2_current = temp2;

				temp1=(q31_t)ADC1->JDR1;
				i16_ph1_current = -i16_ph2_current-temp1;

			}
			break;
		case 3: //Phase B at high dutycycles, read from A+C (B=-A-C)
			{
				temp1=(q31_t)ADC1->JDR1;
				i16_ph1_current = temp1 ;
				temp2=(q31_t)ADC2->JDR1;
				i16_ph2_current = -i16_ph1_current-temp2;
			}
			break;

		case 0: //timeslot too small for ADC
			{
				//do nothing
			}
			break;
		} // end case
#endif


	__disable_irq(); //ENTER CRITICAL SECTION!!!!!!!!!!!!!

	//extrapolate recent rotor position
	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last event
	if (MS.hall_angle_detect_flag) {
		if(ui16_timertics<SIXSTEPTHRESHOLD && ui16_tim2_recent<200)ui8_6step_flag=0;
		if(ui16_timertics>(SIXSTEPTHRESHOLD*6)>>2)ui8_6step_flag=1;


		if(MS.angle_est){
			q31_rotorposition_PLL += q31_angle_per_tic;
		}
		if (ui16_tim2_recent < ui16_timertics+(ui16_timertics>>2) && !ui8_overflow_flag && !ui8_6step_flag) { //prevent angle running away at standstill
			if(MS.angle_est&&iabs(q31_PLL_error)<deg_30){
				q31_rotorposition_absolute=q31_rotorposition_PLL;
				MS.system_state=PLL;
			}
		else{
			q31_rotorposition_absolute = q31_rotorposition_hall
					+ (q31_t) (i8_recent_rotor_direction
							* ((10923 * ui16_tim2_recent) / ui16_timertics)
							<< 16); //interpolate angle between two hallevents by scaling timer2 tics, 10923<<16 is 715827883 = 60deg
			MS.system_state=Interpolation;
			}
		} else {
			ui8_overflow_flag = 1;
			if(MS.KV_detect_flag)q31_rotorposition_absolute = q31_rotorposition_hall;
			else q31_rotorposition_absolute = q31_rotorposition_hall+i8_direction*deg_30;//offset of 30 degree to get the middle of the sector
			MS.system_state=SixStep;
				//	}

		}
	} //end if hall angle detect
	//temp2=(((q31_rotorposition_absolute >> 23) * 180) >> 8);
	__enable_irq(); //EXIT CRITICAL SECTION!!!!!!!!!!!!!!

#ifndef DISABLE_DYNAMIC_ADC

	//get the Phase with highest duty cycle for dynamic phase current reading
	dyn_adc_state(q31_rotorposition_absolute);
	//set the according injected channels to read current at Low-Side active time

	if (MS.char_dyn_adc_state!=char_dyn_adc_state_old){
		set_inj_channel(MS.char_dyn_adc_state);
		char_dyn_adc_state_old = MS.char_dyn_adc_state;
		}
#endif

	//int16_current_target=0;
	// call FOC procedure if PWM is enabled

	if (READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){
	FOC_calculation(i16_ph1_current, i16_ph2_current, q31_rotorposition_absolute, (((int16_t)i8_direction*i8_reverse_flag)*MS.i_q_setpoint), &MS);
	}
	//temp5=__HAL_TIM_GET_COUNTER(&htim1);
	//set PWM

	TIM1->CCR1 =  (uint16_t) switchtime[0];
	TIM1->CCR2 =  (uint16_t) switchtime[1];
	TIM1->CCR3 =  (uint16_t) switchtime[2];
	//__enable_irq();


	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	} // end else

}

void hall_event(void)
{
	 //__HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter

		ui16_timertics = TIM2->CCR1;


	//Hall sensor event processing

		ui8_hall_state = GPIOA->IDR & 0b111; //Mask input register with Hall 1 - 3 bits


		ui8_hall_case=ui8_hall_state_old*10+ui8_hall_state;
		if(MS.hall_angle_detect_flag){ //only process, if autodetect procedere is fininshed
		ui8_hall_state_old=ui8_hall_state;
		}

			uint32_tics_filtered-=uint32_tics_filtered>>3;
			uint32_tics_filtered+=ui16_timertics;

		   ui8_overflow_flag=0;
		   ui8_SPEED_control_flag=1;



		switch (ui8_hall_case) //12 cases for each transition from one stage to the next. 6x forward, 6x reverse
				{
			//6 cases for forward direction
		//6 cases for forward direction
		case 64:
			q31_rotorposition_hall = Hall_64;

			i8_recent_rotor_direction = -i16_hall_order;
			uint16_full_rotation_counter = 0;
			break;
		case 45:
			q31_rotorposition_hall = Hall_45;

			i8_recent_rotor_direction = -i16_hall_order;
			break;
		case 51:
			q31_rotorposition_hall = Hall_51;

			i8_recent_rotor_direction = -i16_hall_order;
			break;
		case 13:
			q31_rotorposition_hall = Hall_13;

			i8_recent_rotor_direction = -i16_hall_order;
			uint16_half_rotation_counter = 0;
			break;
		case 32:
			q31_rotorposition_hall = Hall_32;

			i8_recent_rotor_direction = -i16_hall_order;
			break;
		case 26:
			q31_rotorposition_hall = Hall_26;

			i8_recent_rotor_direction = -i16_hall_order;
			break;

			//6 cases for reverse direction
		case 46:
			q31_rotorposition_hall = Hall_64;

			i8_recent_rotor_direction = i16_hall_order;
			break;
		case 62:
			q31_rotorposition_hall = Hall_26;

			i8_recent_rotor_direction = i16_hall_order;
			break;
		case 23:
			q31_rotorposition_hall = Hall_32;

			i8_recent_rotor_direction = i16_hall_order;
			uint16_half_rotation_counter = 0;
			break;
		case 31:
			q31_rotorposition_hall = Hall_13;

			i8_recent_rotor_direction = i16_hall_order;
			break;
		case 15:
			q31_rotorposition_hall = Hall_51;

			i8_recent_rotor_direction = i16_hall_order;
			break;
		case 54:
			q31_rotorposition_hall = Hall_45;

			i8_recent_rotor_direction = i16_hall_order;
			uint16_full_rotation_counter = 0;
			break;

		} // end case

		if(MS.angle_est){
			q31_PLL_error=q31_rotorposition_PLL-q31_rotorposition_hall;
			q31_angle_per_tic = speed_PLL(q31_rotorposition_PLL,q31_rotorposition_hall,0);
		}

	#ifdef SPEED_PLL
		if(ui16_erps>30){   //360 interpolation at higher erps
			if(ui8_hall_case==32||ui8_hall_case==23){
				q31_angle_per_tic = speed_PLL(q31_rotorposition_PLL,q31_rotorposition_hall, SPDSHFT*tics_higher_limit/(uint32_tics_filtered>>3));

			}
		}
		else{

			q31_angle_per_tic = speed_PLL(q31_rotorposition_PLL,q31_rotorposition_hall, SPDSHFT*tics_higher_limit/(uint32_tics_filtered>>3));
		}

	#endif



}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	//PAS processing
	if(GPIO_Pin == PAS_EXTI8_Pin)
	{
		ui8_PAS_flag = 1;
	}

	//Speed processing
	if(GPIO_Pin == Speed_EXTI5_Pin)
	{

			ui8_SPEED_flag = 1; //with debounce

	}

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_UART_flag=1;

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_UART_TxCplt_flag=1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle) {
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
       KingMeter_Init (&KM);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
       Bafang_Init (&BF);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
       kunteng_init();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
//       ebics_init();
#endif

}



#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
void kingmeter_update(void)
{
    /* Prepare Tx parameters */

    if(battery_percent_fromcapacity > 10)
    {
        KM.Tx.Battery = KM_BATTERY_NORMAL;
    }
    else
    {
        KM.Tx.Battery = KM_BATTERY_LOW;
    }


#if (SPEEDSOURCE  == EXTERNAL)
    	KM.Tx.Wheeltime_ms = ((MS.Speed>>3)*PULSES_PER_REVOLUTION); //>>3 because of 8 kHz counter frequency, so 8 tics per ms
#else
        if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
        {
    	KM.Tx.Wheeltime_ms = (MS.Speed*GEAR_RATIO*6)>>9; //>>9 because of 500kHZ timer2 frequency, 512 tics per ms should be OK *6 because of 6 hall interrupts per electric revolution.

    }
    else
    {
        KM.Tx.Wheeltime_ms = 64000;
    }

#endif
    if(MS.Temperature<130) KM.Tx.Error = KM_ERROR_NONE;
    else KM.Tx.Error = KM_ERROR_OVHT;

    KM.Tx.Current_x10 = (uint16_t) (MS.Battery_Current/100); //MS.Battery_Current is in mA


    /* Receive Rx parameters/settings and send Tx parameters */
    KingMeter_Service(&KM);


    /* Apply Rx parameters */

    MS.assist_level = KM.Rx.AssistLevel;

    if(KM.Rx.Headlight == KM_HEADLIGHT_OFF)
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        }
        else // KM_HEADLIGHT_ON, KM_HEADLIGHT_LOW, KM_HEADLIGHT_HIGH
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }


    if(KM.Rx.PushAssist == KM_PUSHASSIST_ON)
    {
    	ui8_Push_Assist_flag=1;
    }
    else
    {
    	ui8_Push_Assist_flag=0;
    }



}

#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
void bafang_update(void)
{
    /* Prepare Tx parameters */

	if(MS.Voltage*CAL_BAT_V>BATTERY_LEVEL_5)battery_percent_fromcapacity=75;
	else if(MS.Voltage*CAL_BAT_V>BATTERY_LEVEL_4)battery_percent_fromcapacity=50;
	else if(MS.Voltage*CAL_BAT_V>BATTERY_LEVEL_3)battery_percent_fromcapacity=30;
	else if(MS.Voltage*CAL_BAT_V>BATTERY_LEVEL_2)battery_percent_fromcapacity=10;
	else if(MS.Voltage*CAL_BAT_V>BATTERY_LEVEL_1)battery_percent_fromcapacity=5;
	else battery_percent_fromcapacity=0;


    	BF.Tx.Battery = battery_percent_fromcapacity;


    if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
    {
#if (SPEEDSOURCE == EXTERNAL)    // Adapt wheeltime to match displayed speedo value according config.h setting
        BF.Tx.Wheeltime_ms = WHEEL_CIRCUMFERENCE*216/(MS.Speed*PULSES_PER_REVOLUTION); // Geschwindigkeit ist Weg pro Zeit Radumfang durch Dauer einer Radumdrehung --> Umfang * 8000*3600/(n*1000000) * Skalierung Bafang Display 200/26,6
#else
        BF.Tx.Wheeltime_ms = internal_tics_to_speedx100(MS.Speed); //missing factor has to be found
#endif
        }
    else
    {
        BF.Tx.Wheeltime_ms = 0; //64000;
    }


       BF.Tx.Power = (MS.Battery_Current/500)&0xFF; // Unit: 1 digit --> 0.5 A, MS.Battery_Current is in mA


    /* Receive Rx parameters/settings and send Tx parameters */
    Bafang_Service(&BF,1);



    /* Apply Rx parameters */

//No headlight supported on my controller hardware.
    if(BF.Rx.Headlight)
    {
    	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);

    }
    else
    {
    	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);

    }


    if(BF.Rx.PushAssist) ui8_Push_Assist_flag=1;
    else ui8_Push_Assist_flag=0;

    MS.assist_level=BF.Rx.AssistLevel;
}

#endif

int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
  // if input is smaller/bigger than expected return the min/max out ranges value
  if (x < in_min)
    return out_min;
  else if (x > in_max)
    return out_max;

  // map the input to the output range.
  // round up if mapping bigger ranges to smaller ranges
  else  if ((in_max - in_min) > (out_max - out_min))
    return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
  // round down if mapping smaller ranges to bigger ranges
  else
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//assuming, a proper AD conversion takes 350 timer tics, to be confirmed. DT+TR+TS deadtime + noise subsiding + sample time
void dyn_adc_state(q31_t angle){
	if (switchtime[2]>switchtime[0] && switchtime[2]>switchtime[1]){
		MS.char_dyn_adc_state = 1; // -90Â° .. +30Â°: Phase C at high dutycycles
		if(switchtime[2]>1500)TIM1->CCR4 =  switchtime[2]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[0]>switchtime[1] && switchtime[0]>switchtime[2]) {
		MS.char_dyn_adc_state = 2; // +30Â° .. 150Â° Phase A at high dutycycles
		if(switchtime[0]>1500)TIM1->CCR4 =  switchtime[0]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[1]>switchtime[0] && switchtime[1]>switchtime[2]){
		MS.char_dyn_adc_state = 3; // +150 .. -90Â° Phase B at high dutycycles
		if(switchtime[1]>1500)TIM1->CCR4 =  switchtime[1]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}
}

static void set_inj_channel(char state){
	switch (state)
	{
	case 1: //Phase C at high dutycycles, read current from phase A + B
		 {
			 ADC1->JSQR=0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
			 ADC1->JOFR1 = ui16_ph1_offset;
			 ADC2->JSQR=0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
			 ADC2->JOFR1 = ui16_ph2_offset;


		 }
			break;
	case 2: //Phase A at high dutycycles, read current from phase C + B
			 {
				 ADC1->JSQR=0b00110000000000000000; //ADC1 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC1->JOFR1 = ui16_ph3_offset;
				 ADC2->JSQR=0b00101000000000000000; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
				 ADC2->JOFR1 = ui16_ph2_offset;


			 }
				break;

	case 3: //Phase B at high dutycycles, read current from phase A + C
			 {
				 ADC1->JSQR=0b00100000000000000000; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
				 ADC1->JOFR1 = ui16_ph1_offset;
				 ADC2->JSQR=0b00110000000000000000; //ADC2 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC2->JOFR1 = ui16_ph3_offset;


			 }
				break;


	}


}
uint8_t throttle_is_set(void){
	if(uint16_mapped_throttle > 0)
	{
		return 1;
	}
	else return 0;
}
void autodetect() {
	SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
	MS.hall_angle_detect_flag = 0; //set uq to contstant value in FOC.c for open loop control
	q31_rotorposition_absolute = 1 << 31;
	i16_hall_order = 1;//reset hall order
	MS.i_d_setpoint= 300; //set MS.id to appr. 2000mA
	MS.i_q_setpoint= 0;
//	uint8_t zerocrossing = 0;
//	q31_t diffangle = 0;
	HAL_Delay(5);
	for (i = 0; i < 1080; i++) {
		q31_rotorposition_absolute += 11930465; //drive motor in open loop with steps of 1 deg
		HAL_Delay(5);
		//printf_("%d, %d, %d, %d\n", temp3>>16,temp4>>16,temp5,temp6);

		if (ui8_hall_state_old != ui8_hall_state) {
			printf_("angle: %d, hallstate:  %d, hallcase %d \n",
					(int16_t) (((q31_rotorposition_absolute >> 23) * 180) >> 8),
					ui8_hall_state, ui8_hall_case);

			switch (ui8_hall_case) //12 cases for each transition from one stage to the next. 6x forward, 6x reverse
			{
			//6 cases for forward direction
			case 64:
				Hall_64=q31_rotorposition_absolute;
				break;
			case 45:
				Hall_45=q31_rotorposition_absolute;
				break;
			case 51:
				Hall_51=q31_rotorposition_absolute;
				break;
			case 13:
				Hall_13=q31_rotorposition_absolute;
				break;
			case 32:
				Hall_32=q31_rotorposition_absolute;
				break;
			case 26:
				Hall_26=q31_rotorposition_absolute;
				break;

				//6 cases for reverse direction
			case 46:
				Hall_64=q31_rotorposition_absolute;
				break;
			case 62:
				Hall_26=q31_rotorposition_absolute;
				break;
			case 23:
				Hall_32=q31_rotorposition_absolute;
				break;
			case 31:
				Hall_13=q31_rotorposition_absolute;
				break;
			case 15:
				Hall_51=q31_rotorposition_absolute;
				break;
			case 54:
				Hall_45=q31_rotorposition_absolute;
				break;

			} // end case


			ui8_hall_state_old = ui8_hall_state;
		}
	}

   	CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning
    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;
    MS.hall_angle_detect_flag=1;
    MS.i_d = 0;
    MS.i_q = 0;
    MS.u_d=0;
    MS.u_q=0;
    MS.i_d_setpoint= 0;
    uint32_tics_filtered=1000000;

	HAL_FLASH_Unlock();

	if (i8_recent_rotor_direction == 1) {
		EE_WriteVariable(EEPROM_POS_HALL_ORDER, 1);
		i16_hall_order = 1;
	} else {
		EE_WriteVariable(EEPROM_POS_HALL_ORDER, -1);
		i16_hall_order = -1;
	}
	EE_WriteVariable(EEPROM_POS_HALL_45, Hall_45 >> 16);
	EE_WriteVariable(EEPROM_POS_HALL_51, Hall_51 >> 16);
	EE_WriteVariable(EEPROM_POS_HALL_13, Hall_13 >> 16);
	EE_WriteVariable(EEPROM_POS_HALL_32, Hall_32 >> 16);
	EE_WriteVariable(EEPROM_POS_HALL_26, Hall_26 >> 16);
	EE_WriteVariable(EEPROM_POS_HALL_64, Hall_64 >> 16);

	HAL_FLASH_Lock();

	MS.hall_angle_detect_flag = 1;

    HAL_Delay(5);
    ui8_KV_detect_flag = 30;


}

void get_standstill_position(){
	  HAL_Delay(100);
	  HAL_TIM_IC_CaptureCallback(&htim2); //read in initial rotor position

		switch (ui8_hall_state) {
			//6 cases for forward direction
			case 2:
				q31_rotorposition_hall = Hall_32;
				break;
			case 6:
				q31_rotorposition_hall = Hall_26;
				break;
			case 4:
				q31_rotorposition_hall = Hall_64;
				break;
			case 5:
				q31_rotorposition_hall = Hall_45;
				break;
			case 1:
				q31_rotorposition_hall = Hall_51;

				break;
			case 3:
				q31_rotorposition_hall = Hall_13;
				break;

			}

			q31_rotorposition_absolute = q31_rotorposition_hall;
}

int32_t speed_to_tics (uint8_t speed){
	return WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*speed*10);
}

int8_t tics_to_speed (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*tics*10);;
}

int16_t internal_tics_to_speedx100 (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*50*3600/(6*GEAR_RATIO*tics);;
}

int16_t external_tics_to_speedx100 (uint32_t tics){
	return WHEEL_CIRCUMFERENCE*8*360/(PULSES_PER_REVOLUTION*tics);;
}

void runPIcontrol(){


		  q31_t_Battery_Current_accumulated -= q31_t_Battery_Current_accumulated>>8;
		  q31_t_Battery_Current_accumulated += ((MS.i_q*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8);

		  MS.Battery_Current = (q31_t_Battery_Current_accumulated>>8)*i8_direction*i8_reverse_flag; //Battery current in mA
		  //Check battery current limit
		  if(MS.Battery_Current>BATTERYCURRENT_MAX) ui8_BC_limit_flag=1;
		  if(MS.Battery_Current<-REGEN_CURRENT_MAX) ui8_BC_limit_flag=1;
		  //reset battery current flag with small hysteresis
		  if(brake_flag==0){
		  //if(HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)){
			  if(((MS.i_q_setpoint*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8)<(BATTERYCURRENT_MAX*7)>>3)ui8_BC_limit_flag=0;
		  }
		  else{
			  if(((MS.i_q_setpoint*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8)>(-REGEN_CURRENT_MAX*7)>>3)ui8_BC_limit_flag=0;
		  }

		  //control iq

		  //if
		  if (!ui8_BC_limit_flag){
			  PI_iq.recent_value = MS.i_q;
			  PI_iq.setpoint = i8_direction*i8_reverse_flag*MS.i_q_setpoint;
		  }
		  else{
			  if(brake_flag==0){
			 // if(HAL_GPIO_ReadPin(Brake_GPIO_Port, Brake_Pin)){
				  PI_iq.recent_value=  (MS.Battery_Current>>6)*i8_direction*i8_reverse_flag;
				  PI_iq.setpoint = (BATTERYCURRENT_MAX>>6)*i8_direction*i8_reverse_flag;
			  	}
			  else{
				  PI_iq.recent_value=  (MS.Battery_Current>>6)*i8_direction*i8_reverse_flag;
				  PI_iq.setpoint = (-REGEN_CURRENT_MAX>>6)*i8_direction*i8_reverse_flag;
			    }
		  }

			  q31_u_q_temp =  PI_control(&PI_iq);

		  //Control id
		  PI_id.recent_value = MS.i_d;
		  PI_id.setpoint = MS.i_d_setpoint;
		  q31_u_d_temp = -PI_control(&PI_id); //control direct current to zero


		  	//limit voltage in rotating frame, refer chapter 4.10.1 of UM1052
		  //MS.u_abs = (q31_t)hypot((double)q31_u_d_temp, (double)q31_u_q_temp); //absolute value of U in static frame
			arm_sqrt_q31((q31_u_d_temp*q31_u_d_temp+q31_u_q_temp*q31_u_q_temp)<<1,&MS.u_abs);
			MS.u_abs = (MS.u_abs>>16)+1;


			if (MS.u_abs > _U_MAX){
				MS.u_q = (q31_u_q_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_d = (q31_u_d_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_abs = _U_MAX;
			}
			else{
				MS.u_q=q31_u_q_temp;
				MS.u_d=q31_u_d_temp;
			}

		  	PI_flag=0;
	  }

q31_t speed_PLL (q31_t ist, q31_t soll, uint8_t speedadapt)
  {
    q31_t q31_p;
    static q31_t q31_d_i = 0;
    static q31_t q31_d_dc = 0;
    temp6 = soll-ist;
    temp5 = speedadapt;
    q31_p=(soll - ist)>>(P_FACTOR_PLL-speedadapt);   				//7 for Shengyi middrive, 10 for BionX IGH3
    q31_d_i+=(soll - ist)>>(I_FACTOR_PLL-speedadapt);				//11 for Shengyi middrive, 10 for BionX IGH3

    //clamp i part to twice the theoretical value from hall interrupts
    if (q31_d_i>((deg_30>>18)*500/ui16_timertics)<<16) q31_d_i = ((deg_30>>18)*500/ui16_timertics)<<16;
    if (q31_d_i<-((deg_30>>18)*500/ui16_timertics)<<16) q31_d_i =- ((deg_30>>18)*500/ui16_timertics)<<16;


    if (!ist&&!soll)q31_d_i=0;

    q31_d_dc=q31_p+q31_d_i;
    return (q31_d_dc);
  }

#if (R_TEMP_PULLUP)
int16_t T_NTC(uint16_t ADC) // ADC 12 Bit, 10k Pullup, Rückgabewert in °C

{
    uint16_t Ux1000 = 3300;
    uint16_t U2x1000 = ADC*Ux1000/4095;
    uint16_t R1 = R_TEMP_PULLUP;
    uint32_t R = U2x1000*R1/(Ux1000-U2x1000);
// 	printf("R= %d\r\n",R);
//  printf("u2= %d\r\n",U2x1000);
     if(R >> 19) return -44;
     uint16_t n = 0;
     while(R >> n > 1) n++;
     	R <<= 13;
     	for(n <<= 6; R >> (n >> 6) >> 13; n++) R -= (R >> 10)*11; // Annäherung 1-11/1024 für 2^(-1/64)
        int16_t T6 = 2160580/(n+357)-1639; // Berechnung für 10 kOhm-NTC (bei 25 °C) mit beta=3900 K
        return (T6 > 0 ? T6+3 : T6-2)/6; // Rundung

}
#endif

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
