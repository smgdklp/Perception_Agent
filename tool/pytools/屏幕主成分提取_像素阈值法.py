import os
import cv2
import numpy as np
from pathlib import Path

# 配置路径
INPUT_DIR = r"D:\CodingPrograms\Perception_Agent\resource\img\Fame_cut\satucut"
OUTPUT_DIR = r"D:\CodingPrograms\Perception_Agent\resource\img\Fame_cut\satucut_result"

def Turn01(imgpath):
    """
    传入图片地址，明度直接转化为np的01二值图
    纯225/2以上变成0，以下变成1
    返回二值np对象 (0 或 1)
    """
    # 读取图片
    img = cv2.imread(imgpath)
    if img is None:
        print(f"❌ 无法读取图片: {imgpath}")
        return None
    
    # 转为灰度图
    if len(img.shape) == 3:
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    else:
        gray = img.copy()
    
    # 硬二分：大于127.5的设为0，小于等于127.5的设为1
    binary = np.where(gray > 127.5, 0, 1).astype(np.uint8)
    
    return binary

def Countmethe(img, threshold=100):
    """
    梯度提取主成分
    img: 二值图 (0或1)
    threshold: 投影阈值，默认100
    
    返回: [x_min, y_min, x_max, y_max] 矩形边界
    """
    # 确保是二值图
    if img.dtype != np.uint8:
        img = img.astype(np.uint8)
    
    # 获取图像尺寸
    h, w = img.shape
    
    # x轴求和（每一列的白点数量，白点=1）
    x_count = np.sum(img == 1, axis=0)  # shape: (w,)
    
    # y轴求和（每一行的白点数量，白点=1）
    y_count = np.sum(img == 1, axis=1)  # shape: (h,)
    
    # 从高位遍历找第一个大于threshold的index (x_max)
    x_max = -1
    for i in range(w - 1, -1, -1):
        if x_count[i] > threshold:
            x_max = i
            break
    
    # 从低位遍历找第一个大于threshold的index (x_min)
    x_min = -1
    for i in range(w):
        if x_count[i] > threshold:
            x_min = i
            break
    
    # 从高位遍历找第一个大于threshold的index (y_max)
    y_max = -1
    for i in range(h - 1, -1, -1):
        if y_count[i] > threshold:
            y_max = i
            break
    
    # 从低位遍历找第一个大于threshold的index (y_min)
    y_min = -1
    for i in range(h):
        if y_count[i] > threshold:
            y_min = i
            break
    
    # 如果没找到有效边界，返回全图范围
    if x_min == -1 or x_max == -1 or y_min == -1 or y_max == -1:
        print(f"⚠️ 未找到有效主成分 (threshold={threshold})，返回全图")
        return [0, 0, w-1, h-1]
    
    # 返回矩形边界 [x_min, y_min, x_max, y_max]
    return [x_min, y_min, x_max, y_max]

def crop_and_save(img_path, rect, output_dir):
    """
    根据矩形边界裁剪并保存图片（直接用原图裁剪）
    """
    # 读取原图（彩色）
    original = cv2.imread(img_path)
    if original is None:
        return False
    
    x_min, y_min, x_max, y_max = rect
    
    # 确保边界在图像范围内
    h, w = original.shape[:2]
    x_min = max(0, x_min)
    y_min = max(0, y_min)
    x_max = min(w-1, x_max)
    y_max = min(h-1, y_max)
    
    # 检查裁剪区域是否有效
    if x_max <= x_min or y_max <= y_min:
        print(f"⚠️ 无效裁剪区域: [{x_min}, {y_min}, {x_max}, {y_max}]")
        return False
    
    # 裁剪
    cropped = original[y_min:y_max+1, x_min:x_max+1]
    
    # 生成保存路径
    filename = Path(img_path).stem
    save_path = os.path.join(output_dir, f"{filename}_cropped.jpg")
    
    # 保存
    cv2.imwrite(save_path, cropped)
    return True

def debug_visualize(binary_img, x_count, y_count, threshold, rect):
    """
    调试可视化（可选）
    """
    try:
        import matplotlib.pyplot as plt
        
        h, w = binary_img.shape
        x_min, y_min, x_max, y_max = rect
        
        plt.figure(figsize=(15, 10))
        
        # 原二值图
        plt.subplot(221)
        plt.imshow(binary_img, cmap='gray')
        plt.title('Binary Image (0=白, 1=黑)')
        plt.axhline(y=y_min, color='r', linestyle='--')
        plt.axhline(y=y_max, color='r', linestyle='--')
        plt.axvline(x=x_min, color='r', linestyle='--')
        plt.axvline(x=x_max, color='r', linestyle='--')
        
        # X轴投影
        plt.subplot(222)
        plt.plot(x_count)
        plt.axhline(y=threshold, color='r', linestyle='--')
        plt.axvline(x=x_min, color='g', linestyle='--')
        plt.axvline(x=x_max, color='g', linestyle='--')
        plt.title(f'X Projection (threshold={threshold})')
        plt.xlabel('X position')
        plt.ylabel('White pixel count')
        
        # Y轴投影
        plt.subplot(223)
        plt.plot(y_count, range(h))
        plt.axhline(y=y_min, color='g', linestyle='--')
        plt.axhline(y=y_max, color='g', linestyle='--')
        plt.axvline(x=threshold, color='r', linestyle='--')
        plt.title(f'Y Projection (threshold={threshold})')
        plt.xlabel('White pixel count')
        plt.ylabel('Y position')
        
        # 矩形边界标记图
        plt.subplot(224)
        plt.imshow(binary_img, cmap='gray')
        plt.title(f'Rect: [{x_min}, {y_min}, {x_max}, {y_max}]')
        plt.axhline(y=y_min, color='r', linestyle='--', linewidth=2)
        plt.axhline(y=y_max, color='r', linestyle='--', linewidth=2)
        plt.axvline(x=x_min, color='r', linestyle='--', linewidth=2)
        plt.axvline(x=x_max, color='r', linestyle='--', linewidth=2)
        
        plt.tight_layout()
        plt.show()
        
    except ImportError:
        print("⚠️ matplotlib未安装，跳过可视化")

def process_images():
    """主处理函数"""
    
    # 1. 创建输出目录
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print(f"✅ 创建输出目录: {OUTPUT_DIR}")
    
    # 2. 获取所有jpg图片
    img_dir = Path(INPUT_DIR)
    image_files = list(img_dir.glob("*.jpg")) + list(img_dir.glob("*.JPG"))
    image_files += list(img_dir.glob("*.png")) + list(img_dir.glob("*.PNG"))
    
    if not image_files:
        print(f"⚠️ 在 {INPUT_DIR} 中未找到图片文件")
        return
    
    print(f"\n📁 找到 {len(image_files)} 张图片")
    print("🔄 开始处理...\n" + "="*50)
    
    success_count = 0
    fail_count = 0
    
    for idx, img_path in enumerate(image_files, 1):
        try:
            print(f"\n🏁 [{idx}/{len(image_files)}] 处理: {img_path.name}")
            
            # Step 1: 二值化 (返回0和1)
            binary_img = Turn01(str(img_path))
            if binary_img is None:
                fail_count += 1
                continue
            print(f"  ✅ 二值化完成 (shape: {binary_img.shape})")
            print(f"  📊 白点(1)数量: {np.sum(binary_img == 1)}")
            print(f"  📊 黑点(0)数量: {np.sum(binary_img == 0)}")
            
            # Step 2: 提取主成分矩形
            rect = Countmethe(binary_img, threshold=100)
            x_min, y_min, x_max, y_max = rect
            print(f"  📐 主成分边界: x:[{x_min}, {x_max}], y:[{y_min}, {y_max}]")
            print(f"  📏 裁剪尺寸: {x_max-x_min+1} x {y_max-y_min+1}")
            
            # 可选：显示调试可视化
            # debug_visualize(binary_img, np.sum(binary_img == 1, axis=0), 
            #                np.sum(binary_img == 1, axis=1), 100, rect)
            
            # Step 3: 裁剪并保存
            if crop_and_save(str(img_path), rect, OUTPUT_DIR):
                success_count += 1
                print(f"  💾 保存成功: {img_path.stem}_cropped.jpg")
            else:
                fail_count += 1
                print(f"  ❌ 裁剪失败")
            
        except Exception as e:
            fail_count += 1
            print(f"  ❌ 处理出错: {e}")
            import traceback
            traceback.print_exc()
    
    print("\n" + "="*50)
    print("🎉 处理完成！")
    print(f"✅ 成功: {success_count} 张")
    print(f"❌ 失败: {fail_count} 张")
    print(f"📂 输出目录: {OUTPUT_DIR}")

if __name__ == "__main__":
    process_images()