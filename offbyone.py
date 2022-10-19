from math import floor

h = 30
n = 40

for i in range(n):
    print(f"thread {i} owns rows [{floor((h-2)*(i/(n))) + 1}, {floor((h-2)*((i+1)/(n))) + 1})")