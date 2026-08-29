import os
import cv2
import numpy as np
from pathlib import Path
import random

# 配置路径
IMG_DIR = r"D:\CodingPrograms\Intergration\Screen_Capture\img"
OUTPUT_BASE = os.path.join(IMG_DIR, "Fame_cut")
LIGHT_GRAD_DIR = os.path.join(OUTPUT_BASE, "Light_Grad")
SATUCUT_DIR = os.path.join(OUTPUT_BASE, "satucut")

def nomalcut(img1, img2, x=5):
    """
    亮度差异切割
    img1, img2: 彩色图像或灰度图像
    x: 亮度差异阈值
    返回: 二值图像 (0或255)
    """
    # 转为灰度图（二维明度图）
    if len(img1.shape) == 3:
        gray1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
    else:
        gray1 = img1.copy()
    
    if len(img2.shape) == 3:
        gray2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)
    else:
        gray2 = img2.copy()
    
    # 计算像素差绝对值
    diff = cv2.absdiff(gray1, gray2)
    
    # 生成二值掩膜：差小于x为255，否则为0
    img3 = np.where(diff < x, 255, 0).astype(np.uint8)
    
    return img3

def satucut(img1, img2, x=5):
    """
    饱和度差异切割
    img1, img2: 彩色图像
    x: 饱和度差异阈值
    返回: 二值图像 (0或255)
    """
    # 转为HSV色彩空间
    hsv1 = cv2.cvtColor(img1, cv2.COLOR_BGR2HSV)
    hsv2 = cv2.cvtColor(img2, cv2.COLOR_BGR2HSV)
    
    # 提取饱和度通道 (H:0, S:1, V:2)
    sat1 = hsv1[:, :, 1].astype(np.float32)
    sat2 = hsv2[:, :, 1].astype(np.float32)
    
    # 饱和度映射到0-255（实际HSV中饱和度范围是0-255）
    # 已经是0-255范围，直接使用
    sat1 = sat1.astype(np.uint8)
    sat2 = sat2.astype(np.uint8)
    
    # 计算像素差绝对值
    diff = cv2.absdiff(sat1, sat2)
    
    # 生成二值掩膜：差小于x为255，否则为0
    img3 = np.where(diff < x, 255, 0).astype(np.uint8)
    
    return img3

def get_image_pairs(image_list, num_pairs=20):
    """
    获取图片配对（随机两两组合）
    """
    pairs = []
    for _ in range(num_pairs):
        # 随机选两张不同的图片
        if len(image_list) < 2:
            print("⚠️ 图片太少，无法配对")
            return []
        
        pair = random.sample(image_list, 2)
        pairs.append(pair)
    
    return pairs

def process_images():
    """主处理函数"""
    
    # 1. 创建输出目录
    os.makedirs(LIGHT_GRAD_DIR, exist_ok=True)
    os.makedirs(SATUCUT_DIR, exist_ok=True)
    print(f"✅ 创建目录: {LIGHT_GRAD_DIR}")
    print(f"✅ 创建目录: {SATUCUT_DIR}")
    
    # 2. 获取所有jpg图片
    img_dir = Path(IMG_DIR)
    jpg_files = list(img_dir.glob("*.jpg")) + list(img_dir.glob("*.JPG"))
    jpg_files += list(img_dir.glob("*.png")) + list(img_dir.glob("*.PNG"))  # 也支持png
    
    if not jpg_files:
        print("⚠️ 没找到图片文件，检查下路径？")
        return
    
    print(f"\n📁 找到 {len(jpg_files)} 张图片")
    print(f"🔄 将进行 20 次随机配对处理\n" + "="*50)
    
    # 3. 获取20组随机配对
    pairs = get_image_pairs(jpg_files, 20)
    
    if not pairs:
        return
    
    # 4. 处理每一对
    for round_num, (img1_path, img2_path) in enumerate(pairs, 1):
        try:
            print(f"\n🏁 第 {round_num}/20 对处理:")
            print(f"  📸 {img1_path.name} vs {img2_path.name}")
            
            # 读取图片
            img1 = cv2.imread(str(img1_path))
            img2 = cv2.imread(str(img2_path))
            
            if img1 is None or img2 is None:
                print(f"  ❌ 读取失败，跳过")
                continue
            
            # 确保两张图片尺寸一致（取较小的尺寸）
            h1, w1 = img1.shape[:2]
            h2, w2 = img2.shape[:2]
            
            if (h1, w1) != (h2, w2):
                # 尺寸不同时，缩放到较小尺寸
                min_h = min(h1, h2)
                min_w = min(w1, w2)
                img1 = cv2.resize(img1, (min_w, min_h))
                img2 = cv2.resize(img2, (min_w, min_h))
                print(f"  🔄 尺寸统一为: {min_h}x{min_w}")
            
            # --- nomalcut处理 ---
            print(f"  🔪 执行 nomalcut (阈值=5)...")
            img3_light = nomalcut(img1, img2, x=5)
            
            # 保存nomalcut结果
            save_name_light = f"light_round{round_num:02d}.jpg"
            save_path_light = os.path.join(LIGHT_GRAD_DIR, save_name_light)
            cv2.imwrite(save_path_light, img3_light)
            print(f"  ✅ nomalcut 保存: {save_name_light}")
            
            # --- satucut处理 ---
            print(f"  🎨 执行 satucut (阈值=5)...")
            img3_satu = satucut(img1, img2, x=5)
            
            # 保存satucut结果
            save_name_satu = f"satu_round{round_num:02d}.jpg"
            save_path_satu = os.path.join(SATUCUT_DIR, save_name_satu)
            cv2.imwrite(save_path_satu, img3_satu)
            print(f"  ✅ satucut 保存: {save_name_satu}")
            
        except Exception as e:
            print(f"  ❌ 第 {round_num} 对处理出错: {e}")
            continue
    
    print("\n" + "="*50)
    print("🎉 全部处理完成！")
    print(f"📂 Light_Grad 输出: {LIGHT_GRAD_DIR}")
    print(f"📂 satucut 输出: {SATUCUT_DIR}")
    
    # 统计信息
    light_count = len([f for f in os.listdir(LIGHT_GRAD_DIR) if f.endswith('.jpg')])
    satu_count = len([f for f in os.listdir(SATUCUT_DIR) if f.endswith('.jpg')])
    print(f"📊 Light_Grad: {light_count} 张图片")
    print(f"📊 satucut: {satu_count} 张图片")

if __name__ == "__main__":
    process_images()