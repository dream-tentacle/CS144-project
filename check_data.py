# format:
# [1731571134.091397] 64 bytes from 2600:9000:2146:e600:0:91c:a40:93a1: icmp_seq=1 ttl=48 time=202 ms

with open("data.txt") as f:
    data = f.readlines()[1:]

max_seq = data[-1].split("icmp_seq=")[1].split(" ")[0]
max_seq = int(max_seq)
print(max_seq)
total_missing = 0
seqs = [False for _ in range(max_seq + 1)]
for line in data:
    seq = int(line.split("icmp_seq=")[1].split(" ")[0])
    seqs[seq] = True

for i, seq in enumerate(seqs[1:]):
    if not seq:
        total_missing += 1
        print(f"Missing seq: {i}")

print(f"Total missing: {total_missing}")

max_consucutive = 0
current_consucutive = 0
for seq in seqs[1:]:
    if seq:
        current_consucutive += 1
    else:
        max_consucutive = max(max_consucutive, current_consucutive)
        current_consucutive = 0

print(f"Max consucutive: {max_consucutive}")

max_rtt = 0
min_rtt = 1000
for line in data:
    rtt = int(line.split("time=")[1].split(" ")[0])
    max_rtt = max(max_rtt, rtt)
    min_rtt = min(min_rtt, rtt)

print(f"Max RTT: {max_rtt}")
print(f"Min RTT: {min_rtt}")

# 统计DUP的次数
dups = 0
for line in data:
    if "DUP" in line:
        dups += 1
print(f"Total DUP: {dups} / {max_seq}")

import pandas

# 绘制RTT随时间变化的图
import matplotlib.pyplot as plt

rtts = []
for line in data:
    rtt = int(line.split("time=")[1].split(" ")[0])
    rtts.append(rtt)

# Make a histogram or Cumulative Distribution Function of the distribution of RTTs
plt.hist(rtts, bins=300)
plt.show()

# make a graph of the correlation between  “RTT of ping #N” and “RTT of ping #N+1”.
# The  x-axis  should  be  the  number  of  milliseconds  from  the  first  RTT,  and  the  y-axis should be the number of milliseconds from the second RTT.

rtt1 = []
rtt2 = []
for i in range(len(rtts) - 1):
    rtt1.append(rtts[i])
    rtt2.append(rtts[i + 1])

# 在点多的地方，颜色越深
plt.hexbin(rtt1, rtt2, gridsize=100, cmap="Greens")
plt.colorbar()
plt.show()

# 绘制DUP的次数随时间变化的图
dups = []
tot_dups = 0
for line in data:
    if "DUP" in line:
        tot_dups += 1
    dups.append(tot_dups)

plt.plot(dups)
plt.show()
