from crccheck.crc import Crc16SpiFujitsu
import itertools

def calculate_crc16_spifujitsu(data_bytes):
    # 创建CRC-16/SPI-FUJITSU对象
    crc = Crc16SpiFujitsu()
    # 计算CRC校验值
    crc.process(data_bytes)
    return crc.final()

# 定义数据组，组内顺序固定
original_groups = [
    [0x03, 0x0E],         # 组1
    [0x3A, 0x96, 0x9D],   # 组2
    [0x60],               # 组3
    [0x9B, 0x00],         # 组4
    [0x36, 0xAF, 0x6C],   # 组5
    [0x01]                # 组6
]

# 目标CRC值
target_crc = 0x8D53

found_sequences = []
count = 0

# 生成所有可能的组的非空子集（1到6个组）
for r in range(1, len(original_groups) + 1):
    # 对于每个子集大小，生成所有可能的子集
    subsets = itertools.combinations(original_groups, r)
    for subset in subsets:
        # 对于每个子集，生成所有可能的排列
        permutations = itertools.permutations(subset)
        for perm in permutations:
            # 将组内的数据按照固定顺序合并
            data = []
            for group in perm:
                data.extend(group)
            crc_result = calculate_crc16_spifujitsu(bytes(data))
            count += 1
            if crc_result == target_crc:
                sequence = ' '.join(['{:02X}'.format(b) for b in data])
                found_sequences.append((sequence, crc_result))
                print("找到匹配的序列:")
                print(sequence)
                print("CRC校验值: {:04X}".format(crc_result))
                # 如果只需要找到第一个匹配的序列，可以在这里使用break
                # 如果需要找到所有匹配的序列，可以不使用break
                # break  # 如果只需要第一个匹配结果，取消注释这一行
        # 如果在子集的排列中已经找到了匹配且只需要一个结果，可以跳出循环
        # if found_sequences:
        #     break
    # 同样，如果只需要一个结果，可以在这里添加break
    # if found_sequences:
    #     break

print(f"总共尝试的组合数量: {count}")

if not found_sequences:
    print("未找到匹配的序列。")
else:
    print(f"总共找到匹配的序列数量: {len(found_sequences)}")

# 03 0E 3A 96 9D 9B 00 36 AF 6C 01
