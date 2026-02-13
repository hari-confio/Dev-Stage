
#ifndef SRC_APP_SERIAL_H_
#define SRC_APP_SERIAL_H_


#include <em_gpio.h>
#include <AppTimer.h>

#define SERIAL_TX_PORT gpioPortC
#define SERIAL_TX_PIN 6

#define SERIAL_RX_PORT gpioPortC
#define SERIAL_RX_PIN 7

#define SERIAL_TX_PORT1 gpioPortF
#define SERIAL_TX_PIN1 3

#define SERIAL_RX_PORT1 gpioPortF
#define SERIAL_RX_PIN1 4

//#define READ_PORT gpioPortC
//#define READ_PIN 10
//#define WRITE_PORT gpioPortC
//#define WRITE_PIN 11
//#define RS485_ENABLE_PIN gpioPortF
#define RS485_ENABLE_PORT gpioPortC
#define RS485_ENABLE_PIN 11

#define RX_BUFF_LEN 64


typedef enum _SERIAL_RX_STATE_
{
	STATE_HEADER_1	= 1,
	STATE_HEADER_2,
	STATE_VERSION,
	STATE_SEQUENCE_1,
	STATE_SEQUENCE_2,
	STATE_COMMAND,
	STATE_LENGTH_1,
	STATE_LENGTH_2,
	STATE_DATA,
	STATE_CHECKSUM,
	STATE_ERROR,
	STATE_EXIT
} SERIAL_RX_STATE;



typedef enum _RX_DATA_TYPE_
{
	DATA_DPID = 0,
	DATA_TYPE,
	DATA_LEN_1,
	DATA_LEN_2,
	DATA_VAL
}RX_DATA_TYPE;


#define ON 0xFF
#define OFF 0x00

#define RX_HEADER_1			0x55
#define RX_HEADER_2			0xAA
#define RX_VERSION			0x02
#define RX_COMMAND			0x06
#define RX_RESET_CMD        0x03
#define RX_COMMAND_TYPE_ON_OFF			0x01
#define RX_COMMAND_TYPE_LEVEL			0x02
#define RX_CMD_ON			0x01
#define RX_CMD_OFF			0x00

#define TX_HEADER_1			0x55
#define TX_HEADER_2			0xAA
#define TX_VERSION			0x02
#define TX_COMMAND			0x06

#define TX_ON_OFF_LENGTH_1  0x00
#define TX_ON_OFF_LENGTH_2  0x05
#define	TX_ON_OFF_DATA_1	0x01
#define	TX_ON_OFF_DATA_2	0x01
#define	TX_ON_OFF_DATA_3	0x00
#define	TX_ON_OFF_DATA_4	0x01
#define	TX_ON_DATA_5		0x01
#define	TX_OFF_DATA_5		0x00
#define TX_ON_CHK_SUM		0x0E
#define TX_OFF_CHK_SUM		0x0D

#define TX_LEVEL_LENGTH_1  0x00
#define TX_LEVEL_LENGTH_2  0x08

#define	TX_LEVEL_DATA_1		0x02
#define	TX_LEVEL_DATA_2		0x02
#define	TX_LEVEL_DATA_3		0x00
#define	TX_LEVEL_DATA_4		0x04
#define	TX_LEVEL_DATA_5		0x00
#define	TX_LEVEL_DATA_6		0x00
void Serail_Init();

void Serail_Init1();
uint8_t Serial_Get_Current_State();
uint8_t Serial_Get_Current_Level();
void Serial_Set_Value(uint8_t Value,uint8_t ep);
void Process_RX_Data();


extern uint8_t rotaryLevel;
extern SSwTimer time_config_addr;
extern SSwTimer send_one_time_cmd;
extern SSwTimer knob_in_addr_mode;
extern SSwTimer modbus_send_twice;
void InitializeButtonTimer();
void sendFrame(uint8_t *frame);
void create_read_frame();
void knx_frame();
typedef unsigned char byte;
extern void time_config_addr_CB();
extern void send_one_time_cmd_CB();
extern void knob_in_addr_mode_CB();
extern void modbus_send_twice_CB();
void crc_fun(byte newValue) ;
byte CalculateChecksum(byte frame[], byte size) ;
extern uint8_t button_event_happed;
void updateKnobLevelhex(uint8_t updateKnobLevel);
extern uint8_t btn1level;
extern uint8_t btn2level ;
extern uint8_t btn3level ;
void extractDestinationValues(uint8_t destHighByte, uint8_t destLowByte,uint8_t dim_value) ;
void InitializeButton_rx_Timer();
extern uint8_t button_pressed_num ; // Global variable to track button presses
void extract_and_print_group_addresses(uint8_t *buffer, uint8_t length, uint8_t button_num) ;
void extract_and_print_group_addresses_1(uint8_t *buffer, uint8_t length, uint8_t button_num) ;

//JAQUAR DATA
#define MODBUS_FRAME_SIZE 8
extern uint8_t config_nvm_addr;
#endif /* SRC_APP_SERIAL_H_ */
