#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ethercat.h"

// PDOリマッピング手順を関数化
void remap_pdo(uint16 slave) {
    uint16 index;
    uint8 subindex;
    int32 sdo_val;

    // 1. RPDOの無効化
    index = 0x1600; // 例: 0x1600はRPDOのインデックス
    subindex = 0x00;
    sdo_val = 0x0000;
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    // 2. RPDOのマッピング
    subindex = 0x01;
    sdo_val = 0x60400010; // 0x6040: Controlword, 16bit
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    subindex = 0x02;
    sdo_val = 0x60710010; // 0x6071: Target Torque, 16bit
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    // 3. RPDOの有効化
    subindex = 0x00;
    sdo_val = 0x02; // マッピングされたオブジェクトの数
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    // 4. TPDOの無効化
    index = 0x1A00; // 例: 0x1A00はTPDOのインデックス
    subindex = 0x00;
    sdo_val = 0x0000;
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    // 5. TPDOのマッピング
    subindex = 0x01;
    sdo_val = 0x60410010; // 0x6041: Statusword, 16bit
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    subindex = 0x02;
    sdo_val = 0x60640020; // 0x6064: Position Actual Value, 32bit
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);

    // 6. TPDOの有効化
    subindex = 0x00;
    sdo_val = 0x02; // マッピングされたオブジェクトの数
    ec_SDOwrite(slave, index, subindex, FALSE, sizeof(sdo_val), &sdo_val, EC_TIMEOUTRXM);
}

// スレーブを実行可能な状態にする関数
int set_slave_operational(uint16 slave) {
    uint16 controlword = 0x0006; // スイッチオンディスエーブル
    uint16 statusword;

    // スイッチオンディスエーブル状態に設定
    *(uint16*)(ec_slave[slave].outputs) = controlword;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // ステータスワードを確認
    statusword = *(uint16*)(ec_slave[slave].inputs);
    if ((statusword & 0x004F) != 0x0040) {
        return 0; // スイッチオンディスエーブルになっていない
    }

    // レディトゥスイッチオンに設定
    controlword = 0x0007;
    *(uint16*)(ec_slave[slave].outputs) = controlword;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // ステータスワードを確認
    statusword = *(uint16*)(ec_slave[slave].inputs);
    if ((statusword & 0x006F) != 0x0021) {
        return 0; // レディトゥスイッチオンになっていない
    }

    // スイッチオンに設定
    controlword = 0x000F;
    *(uint16*)(ec_slave[slave].outputs) = controlword;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // ステータスワードを確認
    statusword = *(uint16*)(ec_slave[slave].inputs);
    if ((statusword & 0x006F) != 0x0023) {
        return 0; // スイッチオンになっていない
    }

    // オペレーションイネーブルに設定
    controlword = 0x001F;
    *(uint16*)(ec_slave[slave].outputs) = controlword;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // ステータスワードを確認
    statusword = *(uint16*)(ec_slave[slave].inputs);
    if ((statusword & 0x006F) != 0x0027) {
        return 0; // オペレーションイネーブルになっていない
    }

    return 1; // 成功
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // EtherCATネットワークの初期化
        if (ec_init(argv[1])) {
            printf("EtherCAT初期化成功\n");

            // スキャンスレーブ
            if (ec_config_init(FALSE) > 0) {
                printf("%dスレーブが見つかりました\n", ec_slavecount);

                // スレーブごとにPDOリマッピングを実行
                for (int slave = 1; slave <= ec_slavecount; slave++) {
                    remap_pdo(slave);
                }

                // マスターの状態をセーフオペレーショナルに設定
                ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

                // マスターの状態をオペレーショナルに設定
                ec_slave[0].state = EC_STATE_OPERATIONAL;
                ec_writestate(0);
                ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

                if (ec_slave[0].state == EC_STATE_OPERATIONAL) {
                    printf("マスターはオペレーショナル状態です\n");

                    // スレーブを実行可能な状態にする
                    if (set_slave_operational(1)) {
                        printf("スレーブは実行可能状態です\n");

                        // トルク指令値
                        int16 target_torque = 100; // 例: トルク指令値
                        int32 position_actual_value;

                        // PDO通信の開始
                        ec_send_processdata();
                        ec_receive_processdata(EC_TIMEOUTRET);

                        // トルク指令の送信と位置の読み出しをループで行う
                        for (int i = 0; i < 100; i++) {
                            // コントロールワードは出力データの最初の2バイト
                            uint16* controlword_ptr = (uint16*)ec_slave[1].outputs;
                            // トルク指令は出力データの3, 4バイト目 (バイトオフセット2)
                            int16* target_torque_ptr = (int16*)(ec_slave[1].outputs + 2);

                            // ステータスワードは入力データの最初の2バイト
                            uint16* statusword_ptr = (uint16*)ec_slave[1].inputs;
                            // 位置値は入力データの3, 4, 5, 6バイト目 (バイトオフセット2)
                            int32* position_actual_value_ptr = (int32*)(ec_slave[1].inputs + 2);

                            // トルク指令を送信
                            *target_torque_ptr = target_torque;

                            // PDO通信の送受信
                            ec_send_processdata();
                            ec_receive_processdata(EC_TIMEOUTRET);

                            // 位置値の読み出し
                            position_actual_value = *position_actual_value_ptr;

                            // 位置値を表示
                            printf("現在の位置: %d\n", position_actual_value);

                            // 1秒待機
                            usleep(1000000);
                        }
                    } else {
                        printf("スレーブは実行可能状態に入れませんでした\n");
                    }
                } else {
                    printf("マスターはオペレーショナル状態に入れませんでした\n");
                }
            } else {
                printf("スレーブが見つかりません\n");
            }
        } else {
            printf("EtherCAT初期化失敗\n");
        }
    } else {
        printf("使用法: %s <ネットワークインターフェイス>\n", argv[0]);
    }

    // EtherCATネットワークの閉鎖
    ec_close();
    return 0;
}
