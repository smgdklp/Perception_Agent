import os
import cv2
import numpy as np
from pathlib import Path

# 配置路径
IMG_DIR = r"D:\CodingPrograms\Intergration\Screen_Capture\img"
OUTPUT_BASE = os.path.join(IMG_DIR, "Light_Grad")

# 梯度阈值列表
THRESHOLDS = [0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2]

def process_images():
    """主处理函数"""
    
    # 1. 创建输出目录
    for th in THRESHOLDS:
        folder_path = os.path.join(OUTPUT_BASE, str(th))
        os.makedirs(folder_path, exist_ok=True)
        print(f"✅ 创建目录: {folder_path}")
    
    # 2. 获取所有png图片（修正这里！）
    img_dir = Path(IMG_DIR)
    png_files = list(img_dir.glob("*.png")) + list(img_dir.glob("*.PNG"))
    
    if not png_files:
        print("⚠️ 没找到png文件，检查下路径？")
        return
    
    print(f"\n📁 找到 {len(png_files)} 张图片")
    print(f"🔄 将重复处理 20 次\n" + "="*50)
    
    # 3. 重复执行20次
    for round_num in range(1, 21):
        print(f"\n🏁 第 {round_num}/20 轮处理开始...")
        
        for img_path in png_files:
            try:
                # 读取图片（PNG支持透明通道，但cv2读出来还是BGR）
                img = cv2.imread(str(img_path), cv2.IMREAD_COLOR)
                if img is None:
                    print(f"  ❌ 无法读取: {img_path.name}")
                    continue
                
                # 转为单通道亮度（灰度图）
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                
                # 归一化到0-1 (255是最高亮度)
                normalized = gray.astype(np.float32) / 255.0
                
                # 对每个阈值处理
                for th in THRESHOLDS:
                    # 小于阈值的置0，大于等于的保持不变
                    result = normalized.copy()
                    result[result < th] = 0.0
                    
                    # 保存时转回0-255 uint8
                    save_img = (result * 255).astype(np.uint8)
                    
                    # 构建保存路径：Light_Grad/阈值/原文件名_轮次.png（修正后缀！）
                    save_dir = os.path.join(OUTPUT_BASE, str(th))
                    name_without_ext = img_path.stem
                    save_name = f"{name_without_ext}_round{round_num:02d}.png"  # 改成png
                    save_path = os.path.join(save_dir, save_name)
                    
                    # PNG保存无需质量参数，无损压缩
                    cv2.imwrite(save_path, save_img)
                
                print(f"  ✅ {img_path.name} 处理完成 (轮次 {round_num})")
                
            except Exception as e:
                print(f"  ❌ {img_path.name} 出错: {e}")
        
        print(f"🏁 第 {round_num} 轮完成")
    
    print("\n" + "="*50)
    print("🎉 全部处理完成！")
    print(f"📂 输出目录: {OUTPUT_BASE}")
    
    # 统计信息
    total_files = 0
    for th in THRESHOLDS:
        folder = os.path.join(OUTPUT_BASE, str(th))
        if os.path.exists(folder):
            count = len([f for f in os.listdir(folder) if f.endswith('.png')])
            print(f"  📂 {th}: {count} 张图片")
            total_files += count
    print(f"📊 总共生成: {total_files} 张图片")

if __name__ == "__main__":
    process_images()