2024/5/27生成（DS-402 & アプリケーションマニュアル織り込み済み

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ecrt.h>

// EtherCATマスターインスタンス
static ec_master_t *master = NULL;
static ec_domain_t *domain1 = NULL;
static ec_slave_config_t *sc = NULL;

// PDO entry offsets
static unsigned int off_control_word;
static unsigned int off_position_actual_value;
static unsigned int off_digital_inputs;
static unsigned int off_status_word;
static unsigned int off_target_torque;
static unsigned int off_mode_of_operation;

// 定義
#define VENDOR_ID 0x00000002
#define PRODUCT_ID 0x00030924
#define REVISION_NO 0x00000001

// 更新周期（例：1ms）
#define PERIOD_NS (1000000)

void cyclic_task(void)
{
    // Domainデータ
    uint8_t *domain1_pd = ec_domain_data(domain1);

    // 制御ワードとトルク指令を設定
    EC_WRITE_U16(domain1_pd + off_control_word, 0x000F); // 制御ワード（例：運転イネーブル）
    EC_WRITE_S16(domain1_pd + off_target_torque, 100);  // トルク指令（例：100）

    // サイクリック同期トルクモードを設定
    EC_WRITE_S8(domain1_pd + off_mode_of_operation, 0x0A); // モード設定（CST）

    // Process Dataを更新
    ecrt_domain_queue(domain1);
    ecrt_master_send(master);
    ecrt_master_receive(master);
    ecrt_domain_process(domain1);
}

int main(int argc, char **argv)
{
    // EtherCATマスターインスタンスの取得
    master = ecrt_request_master(0);
    if (!master) {
        fprintf(stderr, "Failed to acquire master!\n");
        return -1;
    }

    // ドメインの作成
    domain1 = ecrt_master_create_domain(master);
    if (!domain1) {
        fprintf(stderr, "Failed to create domain!\n");
        return -1;
    }

    // スレーブの構成
    sc = ecrt_master_slave_config(master, 0, VENDOR_ID, PRODUCT_ID);
    if (!sc) {
        fprintf(stderr, "Failed to configure slave!\n");
        return -1;
    }

    // PDOエントリの設定
    ec_pdo_entry_reg_t domain1_regs[] = {
        {0, VENDOR_ID, PRODUCT_ID, 0x6040, 0, &off_control_word},         // Control word
        {0, VENDOR_ID, PRODUCT_ID, 0x6064, 0, &off_position_actual_value}, // Position actual value
        {0, VENDOR_ID, PRODUCT_ID, 0x60FD, 0, &off_digital_inputs},       // Digital Inputs
        {0, VENDOR_ID, PRODUCT_ID, 0x6041, 0, &off_status_word},          // Status word
        {0, VENDOR_ID, PRODUCT_ID, 0x6071, 0, &off_target_torque},        // Target Torque
        {0, VENDOR_ID, PRODUCT_ID, 0x6060, 0, &off_mode_of_operation},    // Modes of Operation
        {}
    };

    if (ecrt_domain_reg_pdo_entry_list(domain1, domain1_regs)) {
        fprintf(stderr, "PDO entry registration failed!\n");
        return -1;
    }

    // マスターのアクティブ化
    if (ecrt_master_activate(master)) {
        fprintf(stderr, "Master activation failed!\n");
        return -1;
    }

    // ドメインプロセスデータの取得
    if (!(domain1_pd = ecrt_domain_data(domain1))) {
        fprintf(stderr, "Failed to get domain data pointer!\n");
        return -1;
    }

    // デバイス初期化シーケンス
    EC_WRITE_U16(domain1_pd + off_control_word, 0x0080); // エラーリセット
    usleep(10000); // 10ms待機
    EC_WRITE_U16(domain1_pd + off_control_word, 0x0006); // スイッチオン準備
    usleep(10000); // 10ms待機
    EC_WRITE_U16(domain1_pd + off_control_word, 0x0007); // スイッチオン
    usleep(10000); // 10ms待機
    EC_WRITE_U16(domain1_pd + off_control_word, 0x000F); // 運転イネーブル
    usleep(10000); // 10ms待機

    // サイクルタスクの実行
    while (1) {
        cyclic_task();
        usleep(PERIOD_NS / 1000);
    }

    return 0;
}

