css_path = r"e:\smart-home-iot\web-dashboard\style.css"
with open(css_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

results = []
for i, line in enumerate(lines):
    if ".slider" in line or "checked" in line:
        results.append(f"{i+1}: {line.strip()}")

with open("css_search.txt", "w", encoding="utf-8") as out:
    for r in results:
        out.write(r + "\n")
print(f"Done. Found {len(results)} matches.")
