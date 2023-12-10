# comperf

用于测试 UART 串口性能的小工具。

- 发送方，按照设定的数据长度和时间间隔发送指定的字符。最后会统计发送的数据量。
- 接收方，接收数据，同时判断数据内容与发送的一致，并计数。最后会统计总接收的数据量，和其中错误的数据量。

发送和接收的数据量一致，且无错误，说明测试成功。

## 语法

```
comperf -d <serial port> [option]
```

- -h, 打印帮助信息。
- -d, 设置串口设备名，例如 /dev/ttyS0 ，必选。
- -b, 设置波特率，数据范围 1200~4000000, 默认是 115200 。
- -p, 设置数据位，校验位和停止位，格式 `<data bit><parity bit><stop bit>`，默认是 8n1 。
    - data bit: 5,6,7,8
    - parity bit: o(odd), e(even), n(no)
    - stop bit: 1,2
- -s/-r, -s 表示发送，-r 表示接收。后面跟一个字节的十六进制数，是发送或接收的字节内容，默认是 0x55 ，可以用示波器测量波特率。
- -i, 每次发送数据的间隔时间，单位是毫秒，默认值是 1000 。
- -l, 每次发送的字节数, 默认值是 10 。接收方要设置同样的参数，用于数据统计。
- -n, 总共发送多少次数据，默认值 0 表示一直发送。接收方要设置同样的参数，用于数据统计。
- -v, 输出信息的级别，默认值是 2 。
    - 0 表示接收和发送过程不打印信息。
    - 1 表示间隔10秒打印一次统计信息。
    - 2 表示每次接收或者发送都会打印信息。

接收：

```
./comperf -d /dev/ttyUSB0  -b 4000000 -r 0x55 -n 5
Linux serial test app

Device: /dev/ttyUSB0
Baud rate: 4000000
Data bit: 8
Parity bit: n
Stop bit: 1
Receive 0x55, 10 bytes per read
Total: 10 * 5 = 50 Bytes
Start ...

-----------------------------------
[1] Receive 10 bytes, error 0 bytes
[2] Receive 10 bytes, error 0 bytes
[3] Receive 10 bytes, error 0 bytes
[4] Receive 10 bytes, error 0 bytes
[5] Receive 10 bytes, error 0 bytes

-----------------------------------
process termination!
Result:
Device: /dev/ttyUSB0
Baud rate: 4000000
Data bit: 8
Parity bit: n
Stop bit: 1
Receive character: 0x55
Received 50 bytes, error 0 bytes
Take time : 6 second
```

发送：

```
./comperf -d /dev/ttyUSB1 -b 4000000 -s 0x55 -n 5 -i 1000
Linux serial test app

Device: /dev/ttyUSB1
Baud rate: 4000000
Data bit: 8
Parity bit: n
Stop bit: 1
Send 0x55, 10 bytes per 1000ms
Total: 10 * 5 = 50 Bytes
Start ...

-----------------------------------
[1] Send 10 bytes 0x55
[2] Send 10 bytes 0x55
[3] Send 10 bytes 0x55
[4] Send 10 bytes 0x55
[5] Send 10 bytes 0x55

-----------------------------------
process termination!
Result:
Device: /dev/ttyUSB1
Baud rate: 4000000
Data bit: 8
Parity bit: n
Stop bit: 1
Send character: 0x55
Sent 50 bytes 0x55
Take time : 5 second
```

## 举例

对于 4000000 波特率的串口，按照 70% 的数据速率计算，即 350000Bytes/s ，可间隔 1ms ，每次发送 350Bytes ，十分钟可以发送 600000 次。收发命令分别是：

```
./comperf -d /dev/ttyUSB0 -b 4000000 -r 0x55  -l 350 -n 600000 -i 1 -v 1
./comperf -d /dev/ttyUSB1 -b 4000000 -s 0x55  -l 350 -n 600000 -i 1 -v 1
```

## 参考

- <https://en.wikibooks.org/wiki/Serial_Programming>
- <https://www.mkssoftware.com/docs/man5/struct_termios.5.asp>
- <https://zhuanlan.zhihu.com/p/521283753>
- <https://blog.csdn.net/qq_21593899/article/details/52281034>
- <https://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html>