import serial
import time
from PIL import Image, ImageOps

# --- 配置 ---
SERIAL_PORT = 'COM4'     # 修改为你的 Arduino Mega 端口号 (Mac/Linux 是 /dev/ttyUSB0 等)
BAUD_RATE = 115200       # 降低波特率测试（从 500000 降到 115200）
IMAGE_PATH = "test.PNG"  # 你想发送的图片
INVERT = False           # 是否反色
THRESHOLD = 128          # 黑白阈值
# -----------

def get_vfd_bytes(image_path):
    """处理图片并返回 1024 字节的数据"""
    try:
        img = Image.open(image_path).convert('L')
        img = img.resize((128, 64), Image.Resampling.LANCZOS)
        
        # 二值化
        fn = lambda x : 255 if x > THRESHOLD else 0
        img = img.point(fn, mode='1')
        
        if INVERT:
            img = ImageOps.invert(img)
            
        # 转换为字节流 (纵向取模，Page 0 -> Page 7)
        data_bytes = bytearray()
        width, height = img.size
        
        for page in range(8):
            for x in range(width):
                byte_val = 0
                for bit in range(8):
                    y = page * 8 + bit
                    if img.getpixel((x, y)) > 0:
                        byte_val |= (1 << bit)
                data_bytes.append(byte_val)
        
        return data_bytes
    except Exception as e:
        print(f"图片处理错误: {e}")
        return None

def main():
    try:
        # 打开串口
        print(f"尝试连接到 {SERIAL_PORT}，波特率: {BAUD_RATE}...")
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5)
        print(f"连接到 {SERIAL_PORT} 成功！")
        
        # 等待 Arduino 重启并初始化
        print("等待 Arduino 初始化 (3秒)...")
        time.sleep(3)
        
        # 清空串口缓冲区
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("串口缓冲区已清空")

        # 获取图片数据
        print(f"正在处理图片: {IMAGE_PATH}")
        img_data = get_vfd_bytes(IMAGE_PATH)
        
        if img_data and len(img_data) == 1024:
            print(f"图片数据生成成功，大小: {len(img_data)} 字节")
            # print(f"前100个字节: {list(img_data[:1024])}")
            print(img_data)
            
            # 发送数据前再次检查串口
            if ser.in_waiting > 0:
                print(f"警告：发送前串口缓冲区有 {ser.in_waiting} 字节数据，已清除")
                ser.read(ser.in_waiting)
            
            start_time = time.time()
            
            print("开始发送数据...")
            ser.write(img_data)
            ser.flush()  # 确保所有数据已发送
            ser.write(img_data)
            ser.flush()  # 确保所有数据已发送
            ser.write(img_data)
            ser.flush()  # 确保所有数据已发送
            
            end_time = time.time()
            print(f"发送完成! 耗时: {(end_time - start_time)*1000:.2f} ms")
            
            # 等待 Arduino 处理并读取回传数据
            print("等待 Arduino 响应...")
            time.sleep(1)
            
            # 读取所有可用数据
            available_bytes = ser.in_waiting
            print(f"Arduino 缓冲区有 {available_bytes} 字节数据")
            
            if available_bytes > 0:
                response = ser.read(available_bytes)
                try:
                    response_str = response.decode('ascii', errors='ignore').strip()
                    print(f"收到 Arduino 回传数据:")
                    print(f"  原始数据: {response}")
                    print(f"  解码文本: {response_str}")
                    
                    # 检查是否收到确认信号 'K'
                    if 'K' in response_str:
                        print("\n✓ 数据传输成功！Arduino 已确认接收。")
                        print("✓ 请查看 VFD 屏幕显示效果。")
                    else:
                        print("\n⚠ 收到数据但未找到确认信号 'K'")
                        print("⚠ 请检查 Arduino 串口监视器输出")
                except Exception as e:
                    print(f"解码失败: {e}")
                    print(f"原始字节: {response}")
            else:
                print("\n✗ 未收到任何回传数据")
                print("✗ 可能的原因:")
                print("  1. 串口波特率不匹配")
                print("  2. Arduino 代码未正确上传")
                print("  3. 串口连接问题")
                print("  4. Arduino 未检测到 1024 字节完整帧")
                
                # 尝试读取调试信息
                print("\n尝试读取 Arduino 调试输出...")
                time.sleep(2)
                if ser.in_waiting > 0:
                    debug_info = ser.read(ser.in_waiting)
                    print(f"调试信息: {debug_info.decode('ascii', errors='ignore')}")
        else:
            print(f"✗ 数据生成失败或长度错误！")
            print(f"  期望长度: 1024 字节")
            print(f"  实际长度: {len(img_data) if img_data else 0} 字节")

        print("\n关闭串口连接...")
        ser.close()
        print("程序结束")

    except serial.SerialException as e:
        print(f"✗ 串口错误: {e}")
        print(f"✗ 无法打开串口 {SERIAL_PORT}")
        print("请检查:")
        print("  1. USB 线是否正确连接")
        print("  2. 串口号是否正确 (在设备管理器中查看)")
        print("  3. 串口是否被其他程序占用")
    except Exception as e:
        print(f"✗ 程序错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
