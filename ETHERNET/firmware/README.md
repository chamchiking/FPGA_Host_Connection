# firmware code

function description

### err_t recv_callback
1. receive a packet
- mode bit (1) => do actions depending on the mode 
- mode1 : print hello world
- mode2 : echo the packet
- mode3 : write data to dram
- mode4 : read data from dram

#### mode3. write data to dram 
4char: offset
4char: data_size

write (data_size) of 8chars starting from the offset 

#### mode4. read data from dram


4char: offset
4char: data_size

read (data_size) of 8chars starting from the offset 
