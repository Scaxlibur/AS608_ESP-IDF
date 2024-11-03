# AS608指纹模块在ESP-IDF环境下的驱动

ESP-IDF版本：v5.3.1

测试用开发板：ESP32-S3 DevkitC-1

使用C++开发

## 功能
实现了模块文档描述的几乎所有功能，包括读/写指纹，读/写记事本、删除指纹数据库等。

默认使用UART1，TXD端口为GPIO17，RXD端口为GPIO18。

## 如何使用

`#include "fingerID.hpp"`

## 文档结构

```
├── CMakeLists.txt
├── .vscode
├── main
│   ├── CMakeLists.txt
│  	├── fingerID.cpp		驱动原文件		
│	├── fingerID.hpp		驱动头文件
│	└── main.c
├── LICENSE
├── .gitignore
└── README.md				你现在正在阅读的
```
