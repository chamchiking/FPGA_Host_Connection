import serial
import glob
import platform
import struct

class UART:
    def __init__(self, device):
        self.ser = serial.Serial(device, 230400)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def su_flush_buffer(self):
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        return None

    # write data with little endian 8byte
    def su_write_raw_data(self, value):
        res = 0
        for i in range(8):
            res = (res) << 8 | ord(value[i])
        byte_num = struct.pack('>Q', res)
        for i in range(8):
            self.ser.write(bytearray([byte_num[i]]))
        return None

    def su_read_data(self, addr):
        read_op = 0x02
        res = (read_op << 60) | (addr << 32)

    def test_hello(self):
        write_op = 0x01
        res = write_op
        byte_num = struct.pack('>b', res)
        if(self.ser.write(bytearray([byte_num[0]]))):
            print("[   OK   ] Mode")
        else:
            print("[ FAILED ] Mode")

        self.su_flush_buffer()

        # temp = self.ser.read(size=1)
        data = self.ser.read(size=16)
        return data

    def test_echo(self, send_data):
        write_op = 0x02
        res = write_op
        byte_num = struct.pack('>b', res)
        if(self.ser.write(bytearray([byte_num[0]]))):
            print("[   OK   ] Mode")
        else:
            print("[ FAILED ] Mode")

        # send 16 byte data
        N = len(send_data)
        first = ""
        second = ""
        for i in range(16):
            send_char = send_data[i] if i < N else " "
            if i < 8:
                first += send_char
            else:
                second += send_char
        self.su_write_raw_data(first)
        self.su_write_raw_data(second)
        

        # read 16 byte data
        self.su_flush_buffer()
        data = self.ser.read(size=16)
        return data
    
    def store_file(self, file_name, base_addr, words_to_send):
        write_op = 0x03
        byte_num = struct.pack('>b', write_op)
        if(self.ser.write(bytearray([byte_num[0]]))):
            print("[   OK   ] Mode")
        else:
            print("[ FAILED ] Mode")

        # checking fpga waiting command
        control_wait = self.ser.read(size=16)
        if control_wait != b'  Waiting_CMD   ':
            print("[ ERROR ] Wait_CMD")
            return None
        print("[  OK  ] Wait_CMD")

        # sending address, data_size
        encoded = (int(hex(words_to_send), 16) << 32) | (base_addr)
        encoded_byte = struct.pack('>Q', encoded)
        for i in range(8):
            self.ser.write(bytearray([encoded_byte[i]]))

        # checking fpga received command
        control_recv = self.ser.read(size=16)
        if control_recv != b'  CMD_Receive   ':
            print(" [ ERROR ] Receive")
            return None
        print("[  OK  ] Receive")

        
        # sending data
        f = open(file_name, 'r')
        for i in range(words_to_send):
            line = f.readline()
            value_byte = bytearray.fromhex(line)
            for i in range(4):
                self.ser.write(bytearray([value_byte[i]]))

        # checking fpga complete command
        control_complete = self.ser.read(size=16)
        print(control_complete)
        return None

    def verify_file(self, base_addr, words_to_recv):
        write_op = 0x04
        byte_num = struct.pack('>b', write_op)
        if(self.ser.write(bytearray([byte_num[0]]))):
            print("[   OK   ] Mode")
        else:
            print("[ FAILED ] Mode")
        
        # checking fpga waiting command
        control_wait = self.ser.read(size=16)
        print(control_wait)
        if control_wait != b'  Waiting_CMD   ':
            print("[ ERROR ] Wait_CMD")
            return None
        print("[  OK  ] Wait_CMD")

        # sending address, data_size
        encoded = (int(hex(words_to_recv), 16) << 32) | (base_addr)
        encoded_byte = struct.pack('>Q', encoded)
        for i in range(8):
            self.ser.write(bytearray([encoded_byte[i]]))
        
        control_recv = self.ser.read(size=16)
        if control_recv != b'  CMD_Receive   ':
            print("[ ERROR ] CMD_Receive")
            return None
        print("[   OK   ] CMD_Receive")

        # receiving data
        data = []
        for i in range(words_to_recv):
            data.append(self.ser.read(size=4))        
        return data
    
    def check_result(self, file_name, recv_data, words_to_check):
        f = open(file_name)
        for i in range(words_to_check):
            row = f.readline().strip().lower()
            if row != recv_data[i].hex().lower():
                print("Not Correct!!")
                return False
        print("Correct!!")
        return True


    def mem_test(self):
        write_op = 0x05
        res = write_op
        byte_num = struct.pack('>b', res)
        if(self.ser.write(bytearray([byte_num[0]]))):
            print("[   OK   ] Mode")
        else:
            print("[ FAILED ] Mode")

        self.su_flush_buffer()

        data = self.ser.read(size=16)
        return data