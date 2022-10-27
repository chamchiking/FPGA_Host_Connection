# Uart Connection

**

## 1. 폴더 구조

```
.
├── firmware                    // 하드웨어 microblaze에 올라가는 코드
│   └── main.c
└── host_pc                     // host 컴퓨터에서 구동하는 코드
    ├── memory_test.ipynb
    ├── testfile.txt
    └── uart.py
```

## 2. 환경

구성에 사용된 환경은 다음과 같다.

1. platform

- Vitis 2021.1

2. FPGA board

- Nexys Video (<https://digilent.com/reference/programmable-logic/nexys-video/start>)
