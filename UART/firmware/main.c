#include <stdio.h>
#include "xil_printf.h"

#include "xparameters.h"
#include "xstatus.h"
#include "xuartlite.h"
#include "xil_io.h"

#define UARTLITE_DEVICE_ID	XPAR_UARTLITE_0_DEVICE_ID

XUartLite UartLite;		/* Instance of the UartLite Device */

/*
 * The following buffers are used in this example to send and receive data
 * with the UartLite.
 */
uint8_t RecvBuffer[100];	/* Buffer for Receiving Data */

uint8_t cHello[16] 	= 	{ ' ', ' ', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', ' '};
uint8_t cWaiting[16] 	= 	{ ' ', ' ', 'W', 'a', 'i', 't', 'i', 'n', 'g', '_', 'C', 'M', 'D', ' ', ' ', ' '};
uint8_t cCmd_recv[16] 	= 	{ ' ', ' ', 'C', 'M', 'D', '_', 'R', 'e', 'c', 'e', 'i', 'v', 'e', ' ', ' ', ' '};
uint8_t cComplete[16]	=	{ ' ', 'S', 't', 'o', 'r', 'e', ' ', 'c', 'o', 'm', 'p', 'l', 'e', 't', 'e', ' '};
uint8_t cMemDone[16]	=	{ ' ', 'M', 'E', 'M', ' ', 'D', 'O', 'N', 'E', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8_t cMemFailed[16]	=	{ ' ', 'M', 'E', 'M', ' ', 'F', 'a', 'i', 'l', 'e', 'd', ' ', ' ', ' ', ' ', ' '};


int init_uart(u16 DeviceID);
void uart_recv(u8 num_bytes);
void uart_send(u8* data, u32 num_bytes);
void write_addr(u32 addr, u8* data, u32 num_bytes);
void read_addr(u32 addr, u8* readbuffer, u32 num_bytes);


int main()
{
	init_uart(UARTLITE_DEVICE_ID);

    while(1) {
		uart_recv(1);
		if(RecvBuffer[0]==0x01){
			uart_send(cHello, 16);
		}
		else if(RecvBuffer[0] ==0x02){
			uart_recv(16);
			uart_send(RecvBuffer, 16);
		}
		else if(RecvBuffer[0] ==0x09){
			uart_recv(8);
			uart_send(RecvBuffer, 8);
		}
		else if(RecvBuffer[0] ==0x03){
			uart_send(cWaiting, 16);
			uart_recv(8);

			uint32_t offset = (RecvBuffer[4] << 24) + (RecvBuffer[5] << 16) + (RecvBuffer[6] << 8) + RecvBuffer[7];
			uint32_t data_size = (RecvBuffer[0] << 24) + (RecvBuffer[1] << 16) + (RecvBuffer[2] << 8) + RecvBuffer[3];

			uart_send(cCmd_recv, 16);

			for(int i=0; i < data_size; i++){
				uart_recv(4);
				write_addr(XPAR_MIG_7SERIES_0_BASEADDR + offset + i*4, RecvBuffer, 4);
			}
			uart_send(cComplete, 16);
		}
		else if(RecvBuffer[0] ==0x04){ // load from DRAM
			// send response
			uart_send(cWaiting, 16);

			//receive base addr and data size
			uart_recv(8);
			uint32_t offset = (RecvBuffer[4] << 24) + (RecvBuffer[5] << 16) + (RecvBuffer[6] << 8) + RecvBuffer[7];
			uint32_t data_size = (RecvBuffer[0] << 24) + (RecvBuffer[1] << 16) + (RecvBuffer[2] << 8) + RecvBuffer[3];

			uart_send(cCmd_recv,16);

			//load from DRAM and send
			uint8_t temp[4];
			for(int i = 0; i < data_size; i++){
				read_addr(XPAR_MIG_7SERIES_0_BASEADDR + offset + i * 4, temp, 4);
				uart_send(temp, 4);
			}
		}
		else if (RecvBuffer[0] ==0x05){
			uint32_t flag = 1;

			uint8_t temp[4];
			for(int i=0; i < 10000; i++){
				temp[0] = i & 0x000000ff;
				write_addr(XPAR_MIG_7SERIES_0_BASEADDR + i*4, temp, 4);
			}

			for(int i = 0; i < 10000; i++){
				read_addr(XPAR_MIG_7SERIES_0_BASEADDR + i * 4, temp, 4);
				if(temp[0]!= (i & 0x000000ff)){
					flag = 0;
				}
			}

			if(flag == 1){
				uart_send(cMemDone, 16);
			} else {
				uart_send(cMemFailed, 16);
			}
		}
//		else if(RecvBuffer[0] ==0x05){ //store to test ip
//			//send response
//			uart_send(cWaiting, 16);
//
//			//receive offset and data
//			uart_recv(8);
//			uint32_t offset = (RecvBuffer[3] << 24) + (RecvBuffer[2] << 16) + (RecvBuffer[1] << 8) + RecvBuffer[0];
//
//			write_addr(XPAR_TEST_IP_0_S00_AXI_BASEADDR + offset, RecvBuffer + 4, 4);
//
//			//send response
//			uart_send(cComplete, 16);
//		} else if(RecvBuffer[0] ==0x06){ // load from test ip
//			//send response
//			uart_send(cWaiting, 16);
//
//			//receive offset
//			uart_recv(4);
//			uint32_t offset = (RecvBuffer[3] << 24) + (RecvBuffer[2] << 16) + (RecvBuffer[1] << 8) + RecvBuffer[0];
//
//			//load data from testip
//			uint8_t temp[4];
//			read_addr(XPAR_TEST_IP_0_S00_AXI_BASEADDR + offset, temp, 4);
//
//			//send via UART
//			uart_send(temp, 4);
//		}
	}

    return 0;
}

int init_uart(u16 DeviceID){
	int status;
	status = XUartLite_Initialize(&UartLite, DeviceID);
	if((status =! XST_SUCCESS)){
		return XST_FAILURE;
	}

	status = XUartLite_SelfTest(&UartLite);
	if ((status = !XST_SUCCESS)){
		return XST_FAILURE;
	}

	XUartLite_ResetFifos(&UartLite);

	return XST_SUCCESS;
}

void uart_recv(u8 num_bytes){
	int received_count = 0;
	while (1) {
		received_count += XUartLite_Recv(&UartLite, RecvBuffer + received_count, num_bytes - received_count);
		if (received_count == num_bytes) {
			break;
		}
	}
	return;
}

void uart_send(u8* data, u32 num_bytes) {
	int sent_bytes = 0;
	while(1){
		sent_bytes += XUartLite_Send(&UartLite, data + sent_bytes, num_bytes - sent_bytes);
		if(sent_bytes == num_bytes){
			break;
		}
	}
	return;
}

void write_addr(u32 addr, u8* data, u32 num_bytes){
	for(int i = 0; i < num_bytes; i++){
		Xil_Out8(addr + i, data[i]);
	}
	return;
}



void read_addr(u32 addr, u8* readbuffer, u32 num_bytes){
	for(int i = 0; i < num_bytes; i++){
		readbuffer[i] = Xil_In8(addr + i);
	}
	return;
}