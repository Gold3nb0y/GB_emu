#include "cart.h"
#include <stdio.h>

void load_cart(cart_t* cart, char* filename){
    FILE* fp;
    char log[0x30];
    memset(log, 0, 0x30);
    snprintf(log, 0x2e, "Cart: %s", filename);
    LOG(INFO, log);
    fp = fopen(filename, "r");
    if(!fp) {
        LOG(ERROR, "failed to open file");
        exit(1);
    }


    fseek(fp, 0x100, SEEK_SET);
    fread(&cart->entry, 0x4, 1, fp);
    fread(&cart->logo, 0x30, 1, fp);
    fread(&cart->title, 0x10, 1, fp);

    memset(log, 0, 0x30);
    snprintf(log, 0x2e, "Title: %s", cart->title);
    LOG(INFO, log);

    fseek(fp, 0x13F, SEEK_SET);
    fread(&cart->manufacturer_code, 0x4, 1, fp);
    fread(&cart->CGB_flag, 1, 1, fp);
    fread(&cart->new_licensee_code, 2, 1, fp);
    fread(&cart->SBG_flag, 1, 1, fp);
    fread(&cart->cart_type, 1, 1, fp);
    fread(&cart->ROM_size, 1, 1, fp);
    fread(&cart->RAM_size, 1, 1, fp);
    fread(&cart->dest_code, 1, 1, fp);
    fread(&cart->old_licensee_code, 1, 1, fp);
    fread(&cart->mask_rom_version_numer, 1, 1, fp);
    fread(&cart->header_checksum, 1, 1, fp);
    fread(&cart->global_checksum, 2, 1, fp);

    return;
}
