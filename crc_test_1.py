from crccheck.crc import Crc16SpiFujitsu

def calculate_crc16_spifujitsu(data_bytes):
    # 创建CRC-16/SPI-FUJITSU对象
    crc = Crc16SpiFujitsu()
    # 计算CRC校验值
    crc.process(data_bytes)
    return crc.final()

# 测试数据
test_data = bytes([0x83,0x42,0x04,0x10,0x04])
crc_result = calculate_crc16_spifujitsu(test_data)
print("CRC-16/SPI-FUJITSU校验值:", hex(crc_result))
