/*
 * Copyright (C) 2009 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "dma_util.h"

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

#include "xzdma.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_io.h"

#define ZDMA_DEVICE_ID		XPAR_XZDMA_1_DEVICE_ID /* ZDMA device Id */
#define ZDMA_INTR_DEVICE_ID	XPAR_XADMAPS_0_INTR/**< ZDMA Interrupt Id */

XZDma_Config *ZDma_Cfg;
XZDma ZDma;

u32 AlloMem[200];  /**< memory allocated for descriptors */

int XZDma_LinearSend(u32 addr, u32 Size){
	int status;
	XZDma_Transfer Data;
	// Connect to the interrupt controller
	status = SetupInterruptSystem(&Intc, &ZDma, ZDMA_INTR_DEVICE_ID);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	xil_printf("1 \n\r");

	// Configuration settings
	XZDma_DataConfig Configure;
	XZDma_GetChDataConfig(&ZDma, &Configure);
	Configure.OverFetch = 0;
	Configure.SrcIssue = 0x1F;
	Configure.SrcBurstType = XZDMA_INCR_BURST;
	Configure.SrcBurstLen = 0xF;
	Configure.DstBurstType = XZDMA_INCR_BURST;
	Configure.DstBurstLen = 0xF;
	if (ZDma_Cfg -> IsCacheCoherent) {
		Configure.SrcCache = 0xF;
		Configure.DstCache = 0xF;
	}
	XZDma_SetChDataConfig(&ZDma, &Configure);

	xil_printf("2 \n\r");

	Data.DstAddr = (UINTPTR)(XPAR_DDR_MEM_BASEADDR + addr);
	Data.DstCoherent = 0;
	Data.Pause = 0;
	Data.SrcAddr = (UINTPTR)SrcBuf;
	Data.SrcCoherent = 0;
	Data.Size = Size;
	if (ZDma_Cfg->IsCacheCoherent) {
		Data.SrcCoherent = 1;
		Data.DstCoherent = 1;
	}

	if(!ZDma_Cfg->IsCacheCoherent) {
		Xil_DCacheFlushRange((INTPTR)Data.SrcAddr, Data.Size);
		Xil_DCacheInvalidateRange((INTPTR)Data.DstAddr, Data.Size);
	}

	xil_printf("3 \n\r");

	XZDma_EnableIntr(&ZDma, (XZDMA_IXR_DMA_DONE_MASK | XZDMA_IXR_DMA_PAUSE_MASK));

	xil_printf("4 \n\r");

	status = XZDma_Start(&ZDma, &Data, 1);
	if(status !=XST_SUCCESS){
		xil_printf("dma send fail");
		return XST_FAILURE;
	}
	xil_printf("5 \n\r");

	while(Done ==0);
	Done = 0;
	xil_printf("6 \n\r");

	XZDma_DisableIntr(&ZDma, (XZDMA_IXR_DMA_DONE_MASK | XZDMA_IXR_DMA_PAUSE_MASK));

	xil_printf("7 \n\r");

	if(!ZDma_Cfg->IsCacheCoherent) {
		Xil_DCacheInvalidateRange((INTPTR)Data.DstAddr, Data.Size);
	}

	xil_printf("8 \n\r");
	XZDma_Reset(&ZDma);
	xil_printf("9 \n\r");

	return XST_SUCCESS;
}

int transfer_data() {
	return 0;
}

void print_app_header()
{
#if (LWIP_IPV6==0)
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
#else
	xil_printf("\n\r\n\r-----lwIPv6 TCP echo server ------\n\r");
#endif
	xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}

uint8_t htoi(const char *hexa)
{
	uint8_t ch = 0;        // 밑의 조건이 안 맞을 때의 기본값
    if('0' <= *hexa && *hexa <= '9')    // 문자 '0'과 '9' 사이라면
        ch = *hexa - '0';             // 문자 '0'의 아스키코드를 빼서 숫자화
    if('A' <= *hexa && *hexa <= 'F')    // 문자 'A'과 'F' 사이라면
        ch = *hexa - 'A' + 10;        // 문자 'A'값을 빼고 10을 더함
    if('a' <= *hexa && *hexa <= 'f')    // 문자 'a'과 'f' 사이라면
        ch = *hexa - 'a' + 10;        // 문자 'a'값을 빼고 10을 더함
    return ch;
}

void itohr(uint8_t* input, char* output){
	for(int i=0; i < 4; i++){
		sprintf((char*)(output+2*i), "%02X", input[i]);
	}
}

uint32_t htoir(const char *hexa)
{
    uint32_t deci = 0;           // 결과를 0으로 초기화
    const char *sp = hexa;  // 16진수 문자열의 주소를 설정
    for(int i=0; i < 8; i++) {
        deci *= 16;         // 결과를 16곱함(16진수이므로)
        char ch = 0;        // 밑의 조건이 안 맞을 때의 기본값
        if('0' <= *sp && *sp <= '9')    // 문자 '0'과 '9' 사이라면
            ch = *sp - '0';             // 문자 '0'의 아스키코드를 빼서 숫자화
        if('A' <= *sp && *sp <= 'F')    // 문자 'A'과 'F' 사이라면
            ch = *sp - 'A' + 10;        // 문자 'A'값을 빼고 10을 더함
        if('a' <= *sp && *sp <= 'f')    // 문자 'a'과 'f' 사이라면
            ch = *sp - 'a' + 10;        // 문자 'a'값을 빼고 10을 더함
        deci += ch;         // 이번 단위의 수를 더함
        sp++;               // 다음 문자
    }
    return deci;
}

char* concat( char*s1, char *s2){
	char * res = malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(res, s1);
	strcpy(res, s2);
	return res;
}

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

	char* data = ((char*) p->payload);
	char mode = *data;
//	xil_printf("%s\n\r\n\r", p->payload);

	if(mode == '1'){
		// send hello world back
		char* cHello = "HELLO WORLD!";
		while (1){
			if(tcp_sndbuf(tpcb) > strlen(cHello)){
				err = tcp_write(tpcb, cHello, strlen(cHello), 1);
				break;
			}
		}
	}
	else if(mode == '2'){
		// echo the packet
		while (1){
			if(tcp_sndbuf(tpcb) > (p->len)-1){
				err = tcp_write(tpcb, (p->payload) +1, (p->len)-1, 1);
				break;
			}
		}
	}
	else if (mode == '3'){
		// write data to dram
		uint32_t offset = htoir(data +1);
		uint32_t data_size = htoir(data + 9);

		// store to dram
		uint8_t temp[4];
		char* start = data + 17;
		for(int i=0; i < data_size; i++){
			temp[0] = htoi(start)*16 + htoi(start+1);
			temp[1] = htoi(start+2)*16 + htoi(start+3);
			temp[2] = htoi(start+4)*16 + htoi(start+5);
			temp[3] = htoi(start+6)*16 + htoi(start+7);
			Xil_Out32(XPAR_DDR_MEM_HIGHADDR - offset - 4*i, (temp[0]<< 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3]);
			start += 8;
		}

//		XZDma_LinearSend(offset, data_size);


		char* cComplete = "PACKET STORED";
		err = tcp_write(tpcb, cComplete, strlen(cComplete), 1);
	}
	else if (mode == '4'){
		// read data from dram

		uint32_t offset = htoir(data +1);
		uint32_t data_size = htoir(data + 9);

		char response[data_size*8 + 1];

		//read from dram
		uint8_t temp[4];
		char hex[8];

		for(int i=0; i < data_size; i++){
			u32 data = Xil_In32(XPAR_DDR_MEM_HIGHADDR - offset - 4*i);
			temp[0] = data >> 24;
			temp[1] = data >> 16;
			temp[2] = data >> 8;
			temp[3] = data;

			itohr(temp, hex);
			memcpy(response + i*8, hex, 8);
		}
//		xil_printf("%s\n\r\n\r", response);
		err = tcp_write(tpcb, response, data_size * 8, 1);
	}

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	/* increment for subsequent accepted connections */
	connection++;

	return ERR_OK;
}


int start_application()
{
//----------------------------------------------------------------------
// ZDMA INITALIZATION
	u32 status;
	ZDma_Cfg = XZDma_LookupConfig(ZDMA_INTC_DEVICE_ID);
	xil_printf("device_id %d\n\r", ZDma_Cfg->DeviceId);
	status = XZDma_CfgInitialize(&ZDma, ZDma_Cfg,ZDma_Cfg->BaseAddress);
	xil_printf("status %d\n\r", status);
	if(status != XST_SUCCESS){
	    xil_printf("DMA initialization failed \n\r");
	    return -1;
	}
	xil_printf("DMA initialization success .... \n\r");

	status = XZDma_SelfTest(&ZDma);
	if(status != XST_SUCCESS){
	    xil_printf("DMA selftest failed \n\r");
	    return -1;
	}
	xil_printf("DMA selftest success .... \n\r");

//	status = XZDma_SetMode(&ZDma, TRUE, XZDMA_NORMAL_MODE);
	status = XZDma_SetMode(&ZDma, FALSE, XZDMA_WRONLY_MODE);
	if(status != XST_SUCCESS){
		xil_printf("DMA write only mode set failure");
	}
	xil_printf("DMA write only mode set success");

	XZDma_CreateBDList(&ZDma, XZDMA_LINEAR, (UINTPTR)AlloMem, 256);


	// Interrupt call back has been set
	XZDma_SetCallBack(&ZDma, XZDMA_HANDLER_DONE, (void*)DoneHandler, &ZDma);
//	XZDma_SetCallBack(&ZDma, XZDMA_HANDLER_ERROR, (void*)ErrorHandler, &ZDma);
	//--------------------------------------------------------------------

	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 7;

	/* create new TCP PCB structure */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ANY_TYPE, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", port);

	return 0;
}
