"""读取 steelball.ndjson，下载图片并画出标注框"""
import json
import urllib.request
import os

NDJSON = r"E:\Downloads\steelball.ndjson"
OUT_DIR = r"E:\Downloads\steelball_viz"
os.makedirs(OUT_DIR, exist_ok=True)

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    os.system("pip install Pillow -q")
    from PIL import Image, ImageDraw, ImageFont

class_names = {}

with open(NDJSON, "r", encoding="utf-8") as f:
    for i, line in enumerate(f):
        obj = json.loads(line.strip())

        if obj["type"] == "dataset":
            class_names = obj.get("class_names", {})
            print(f"数据集: {obj['name']}, 类别: {class_names}")
            continue

        if obj["type"] != "image":
            continue

        # 取前 5 张
        if i > 6:
            break

        url = obj["url"]
        fname = obj["file"]
        w, h = obj["width"], obj["height"]
        boxes = obj.get("annotations", {}).get("boxes", [])

        out_path = os.path.join(OUT_DIR, os.path.splitext(fname)[0] + ".jpg")

        # 下载
        try:
            urllib.request.urlretrieve(url, out_path)
            print(f"[{i}] {fname} ({w}x{h}) {len(boxes)} 个球")
        except Exception as e:
            print(f"[{i}] 下载失败: {e}")
            continue

        # 画框
        img = Image.open(out_path)
        draw = ImageDraw.Draw(img)
        for box in boxes:
            cls_id, cx, cy, bw, bh = box[0], box[1], box[2], box[3], box[4]
            x1 = (cx - bw/2) * w
            y1 = (cy - bh/2) * h
            x2 = (cx + bw/2) * w
            y2 = (cy + bh/2) * h

            name = class_names.get(str(cls_id), f"cls{cls_id}")
            draw.rectangle([x1, y1, x2, y2], outline="green", width=4)
            draw.text((x1, y1 - 15), name, fill="green")

        img.save(out_path)

print(f"\n完成，图片保存在 {OUT_DIR}")
