// // 简单的串口测试程序
// // 用于验证 Arduino Mega 的串口通信是否正常

// void setup() {
//     // 初始化串口
//     Serial.begin(115200);
    
//     // 等待串口准备好
//     delay(2000);
    
//     // 发送启动消息
//     Serial.println("=================================");
//     Serial.println("Arduino Mega 串口测试程序");
//     Serial.println("波特率: 115200");
//     Serial.println("=================================");
//     Serial.println("等待接收数据...");
//     Serial.println("请发送任意字符，我会回显");
//     Serial.println();
// }

// void loop() {
//     // 检查是否有数据到达
//     int available = Serial.available();
    
//     if (available > 0) {
//         Serial.print("收到数据! 字节数: ");
//         Serial.println(available);
        
//         // 读取并回显所有数据
//         while (Serial.available() > 0) {
//             byte data = Serial.read();
//             Serial.print("接收字节: ");
//             Serial.print(data, DEC);
//             Serial.print(" (");
//             if (data >= 32 && data <= 126) {
//                 Serial.print((char)data);
//             } else {
//                 Serial.print("?");
//             }
//             Serial.println(")");
//         }
        
//         Serial.println("数据已处理，等待更多数据...");
//         Serial.println();
//     }
// }
