import time
from pyethercat import Master

# EtherCATマスタを初期化
master = Master(interface="eth0")  # 使用するインターフェース名を指定

# スレーブデバイス（モータードライバ）を取得
slave = master.slaves[0]  # 一つ目のスレーブデバイスを使用

# トルク指令と位置確認のためのPDOインデックスを設定
TORQUE_COMMAND_INDEX = 0x6071  # トルク指令のインデックス
POSITION_ACTUAL_INDEX = 0x6064  # 現在位置のインデックス

def set_torque(torque_value):
    # トルク指令を設定
    slave.sdo_write(TORQUE_COMMAND_INDEX, torque_value)

def get_position():
    # 現在位置を取得
    position = slave.sdo_read(POSITION_ACTUAL_INDEX)
    return position

try:
    # EtherCAT通信を開始
    master.start()

    # トルク指令を送信
    torque_value = 1000  # 例: 1000mNmのトルクを設定
    set_torque(torque_value)
    print(f"Torque command set to: {torque_value}")

    # 一定時間待機してから現在位置を取得
    time.sleep(1)  # 1秒待機
    position = get_position()
    print(f"Current motor position: {position}")

finally:
    # EtherCAT通信を停止
    master.stop()