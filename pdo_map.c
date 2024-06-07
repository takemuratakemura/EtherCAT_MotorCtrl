// PDOマッピングを表示する関数
void print_pdo_mapping(uint16 slave)
{
    int i, j;
    uint16 index;
    int psize;
    uint8 bitlength;
    
    // RxPDOマッピングの表示
    printf("RxPDO Mapping:\n");
    for (i = 0; i < ec_slave[slave].Obytes; i++)
    {
        index = ec_slave[slave].SM[2].StartAddr + i;
        psize = ec_slave[slave].SM[2].SMlength;
        bitlength = ec_slave[slave].Osize[i];
        printf("  Index: 0x%04X, BitLength: %d\n", index, bitlength);
    }

    // TxPDOマッピングの表示
    printf("TxPDO Mapping:\n");
    for (i = 0; i < ec_slave[slave].Ibytes; i++)
    {
        index = ec_slave[slave].SM[3].StartAddr + i;
        psize = ec_slave[slave].SM[3].SMlength;
        bitlength = ec_slave[slave].Isize[i];
        printf("  Index: 0x%04X, BitLength: %d\n", index, bitlength);
    }
}