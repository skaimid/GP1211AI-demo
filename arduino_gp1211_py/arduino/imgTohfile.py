from PIL import Image, ImageOps
import sys
import os

# --- 配置区域 ---
INPUT_IMAGE = "test.PNG"  # 在这里修改你的图片文件名
OUTPUT_NAME = "my_image"  # 生成的数组变量名
INVERT_COLOR = False      # 如果显示反了（底色亮），把这里改成 True
THRESHOLD = 128           # 黑白阈值 (0-255)，越小越容易变白
# ----------------

def convert_to_vfd_array(image_path):
    try:
        # 1. 打开图片
        img = Image.open(image_path).convert('L') # 转为灰度
    except FileNotFoundError:
        print(f"错误: 找不到文件 {image_path}")
        return

    # 2. 调整大小为 128x64
    # 使用抗锯齿缩放
    img = img.resize((128, 64), Image.Resampling.LANCZOS)

    # 3. 二值化 (黑白化)
    # VFD: 1=亮, 0=灭。
    # 通常图片里白色是255。如果 INVERT_COLOR=False，则白色像素会被处理为1(亮)
    fn = lambda x : 255 if x > THRESHOLD else 0
    img = img.point(fn, mode='1')

    if INVERT_COLOR:
        img = ImageOps.invert(img)

    # 4. 开始取模 (纵向取模，按页排列)
    # 对应 C 代码中的: DP_RAM[page][col]
    
    hex_array = []
    width, height = img.size
    
    # 遍历 8 个页 (Page 0 - Page 7)
    for page in range(8):
        for x in range(width):
            byte_val = 0
            # 每一列取 8 个垂直像素
            for bit in range(8):
                y = page * 8 + bit
                # 获取像素值 (0 或 255)
                pixel = img.getpixel((x, y))
                
                # 如果像素是亮的 (255/1)，设置对应的位
                # 根据 ASC57.h 推断，通常 LSB (最低位) 在上面
                if pixel > 0:
                    byte_val |= (1 << bit)
            
            hex_array.append(f"0x{byte_val:02X}")

    # 5. 生成 C 代码输出
    print(f"// 图片尺寸: {width}x{height}")
    print(f"// 数组长度: {len(hex_array)} 字节")
    print(f"// 请将以下代码复制到头文件中 (.h)")
    print("-" * 60)
    print(f"#include <avr/pgmspace.h>\n")
    print(f"const unsigned char {OUTPUT_NAME}[] PROGMEM = {{")
    
    # 格式化输出，每行 16 个字节
    for i in range(0, len(hex_array), 16):
        line = ", ".join(hex_array[i:i+16])
        print(f"    {line},")
        
    print("};")
    print("-" * 60)
    
    # 可选：显示预览图
    print("预览图已显示 (关闭窗口以退出)")
    img.show()

if __name__ == "__main__":
    # 如果命令行传入了参数，优先使用参数
    if len(sys.argv) > 1:
        INPUT_IMAGE = sys.argv[1]
        
    convert_to_vfd_array(INPUT_IMAGE)