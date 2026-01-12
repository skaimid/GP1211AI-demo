import serial
import time

# --- 配置 ---
SERIAL_PORT = 'COM4'     # 修改为你的 Arduino Mega 端口号
BAUD_RATE = 115200       # 波特率
# -----------

def test_serial_communication():
    """测试串口通信是否正常"""
    try:
        print("=" * 50)
        print("串口通信测试程序")
        print("=" * 50)
        
        # 1. 列出可用的串口
        print("\n可用串口列表:")
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"  {port.device}: {port.description}")
        
        print(f"\n尝试连接到 {SERIAL_PORT}，波特率: {BAUD_RATE}...")
        
        # 2. 打开串口
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=3)
        print(f"✓ 成功打开串口 {SERIAL_PORT}")
        
        # 3. 清空缓冲区
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # 4. 等待 Arduino 启动并读取启动消息
        print("\n等待 Arduino 启动 (3秒)...")
        time.sleep(3)
        
        # 5. 读取 Arduino 发送的启动消息
        if ser.in_waiting > 0:
            startup_msg = ser.read(ser.in_waiting).decode('ascii', errors='ignore')
            print(f"\n收到 Arduino 启动消息:")
            print("-" * 50)
            print(startup_msg)
            print("-" * 50)
        else:
            print("\n✗ 未收到 Arduino 启动消息")
            print("可能原因:")
            print("  1. test_serial.ino 未正确上传")
            print("  2. Arduino Mega 串口选择错误 (Serial, Serial1, Serial2, Serial3)")
            print("  3. USB 连接问题")
            ser.close()
            return False
        
        # 6. 发送测试数据
        test_data = b"Hello Arduino!\r\n"
        print(f"\n发送测试数据: {test_data}")
        ser.write(test_data)
        ser.flush()
        
        # 7. 等待并读取响应
        time.sleep(1)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting).decode('ascii', errors='ignore')
            print(f"\n收到 Arduino 响应:")
            print("-" * 50)
            print(response)
            print("-" * 50)
            
            if "收到数据" in response:
                print("\n✓ 串口通信测试成功！")
                print("✓ Arduino 可以正常接收和发送数据")
                return True
            else:
                print("\n⚠ 收到响应但格式不符合预期")
                return False
        else:
            print("\n✗ 未收到 Arduino 响应")
            print("Arduino 接收到数据但没有回显")
            return False
            
    except serial.SerialException as e:
        print(f"\n✗ 串口错误: {e}")
        print(f"✗ 无法打开串口 {SERIAL_PORT}")
        return False
    except Exception as e:
        print(f"\n✗ 程序错误: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("\n串口连接已关闭")

def diagnose_original_issue():
    """诊断原始代码的问题"""
    print("\n" + "=" * 50)
    print("原始问题诊断")
    print("=" * 50)
    
    print("\n如果串口测试成功，但原始 arduino.ino 不工作，可能原因:")
    print("1. TimerOne 库未安装")
    print("   解决: 在 Arduino IDE 中安装 TimerOne 库")
    print("2. 定时器中断阻塞了 loop() 的执行")
    print("3. SPI 初始化或操作有问题")
    print("4. 上电时序导致 Arduino 卡死")
    print("\n建议:")
    print("  - 先确认 TimerOne 库已安装")
    print("  - 尝试禁用定时器中断，只测试串口接收")
    print("  - 检查 SPI 引脚连接是否正确")

if __name__ == "__main__":
    # 运行串口测试
    success = test_serial_communication()
    
    # 诊断原始问题
    diagnose_original_issue()
    
    print("\n" + "=" * 50)
    print("测试完成")
    print("=" * 50)
