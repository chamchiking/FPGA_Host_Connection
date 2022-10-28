#include <stdio.h>
#include "xaxidma.h"
#include "xaxidma_hw.h"
#include "sleep.h"
#include "xexample.h"

u32 checkHalted(u32 baseAddress, u32 offset);
void write_addr(u32 addr, u32* data, u32 num_data);
void read_addr(u32 addr, u32* readbuffer, u32 num_data);

int main(){
	u32 a[] = {1, 2, 3, 4, 5, 6, 7, 8};
	u32 b[8];
	u32 status;

	XExample_Config *myExampleConfig;
	XExample myExample;
	myExampleConfig = XExample_LookupConfig(XPAR_EXAMPLE_0_DEVICE_ID);
	status = XExample_CfgInitialize(&myExample, myExampleConfig);
	if(status != XST_SUCCESS){
		print("example initialization failed \r\n");
		return -1;
	}
	print("example initialization success .... \r\n");
	XExample_Start(&myExample);
//	XExample_EnableAutoRestart(&myExample);

	//-----------------------------------------------------------------------
	XAxiDma_Config *myDmaConfig;
	XAxiDma myDma;

	myDmaConfig = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);
	status = XAxiDma_CfgInitialize(&myDma, myDmaConfig);
	if(status != XST_SUCCESS){
		print("DMA initialization failed \r\n");
		return -1;
	}
	print("DMA initialization success .... \r\n");
	status = XAxiDma_Selftest(&myDma);
	if(status != XST_SUCCESS){
		print("DMA test failed \r\n");
		return -1;
	}
	print("DMA test success .... \r\n");

	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR, 0x4);
	xil_printf("status before data transfer %0x \r\n", status);

	// 1. Define the base address
//	u32 base_addr = XPAR_MIG_7SERIES_0_BASEADDR;

	// 2. Copy data from CPU to mem a at the base address
	write_addr(XPAR_MIG_7SERIES_0_BASEADDR, a, 8);
	// 3. Start DMA to read data from MEM at the base address <DEVICE TO DMA>
	status = XAxiDma_SimpleTransfer(&myDma, XPAR_MIG_7SERIES_0_BASEADDR, 8* sizeof(u32), XAXIDMA_DEVICE_TO_DMA);
	if(status != XST_SUCCESS){
		print("DMA transfer failed\r\n");
		return -1;
	}
	// 4. Write data from DEVICE To memory <DMA TO DEVICE>
	status = XAxiDma_SimpleTransfer(&myDma, XPAR_MIG_7SERIES_0_BASEADDR, 8* sizeof(u32), XAXIDMA_DMA_TO_DEVICE);
	if(status != XST_SUCCESS){
		print("DMA transfer failed\r\n");
		return -1;
	}
	// 5. Read data to b
	read_addr(XPAR_MIG_7SERIES_0_BASEADDR, b, 8);

//	status = XAxiDma_Started(&myDma);
//	if(status != XST_SUCCESS){
//		print("DMA start failed \r\n");
//		return -1;
//	}
	print("DMA start success ....\r\n");

	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR, 0x4);

//	while(status != 1){
//		status = checkHalted(XPAR_AXI_DMA_0_BASEADDR, 0x4);
//	}
//	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR, 0x34);
//	while(status != 1){
//		status = checkHalted(XPAR_AXI_DMA_0_BASEADDR, 0x34);
//	}

	xil_printf("status after data transfer %0x \r\n", status);
	print("DMA transfer success ...\n");

	for(int i=0; i < 8; i++){
		xil_printf("result %0x \r\n", b[i]);
	}
	return 0;
}

void initializeIP(){

}

u32 checkHalted(u32 baseAddress, u32 offset) {
	u32 status;
	status = (XAxiDma_ReadReg(baseAddress, offset)) & XAXIDMA_HALTED_MASK;
	return status;
}

void write_addr(u32 addr, u32* data, u32 num_data){
	for(int i = 0; i < num_data; i++){
		Xil_Out32(addr + 4* i, data[i]);
	}
	return;
}

void read_addr(u32 addr, u32* readbuffer, u32 num_data){
	for(int i = 0; i < num_data; i++){
		readbuffer[i] = Xil_In32(addr + 4* i);
	}
	return;
}
