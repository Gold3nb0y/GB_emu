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
    fread(&cart->num_ROM, 1, 1, fp);
    fread(&cart->val_RAM, 1, 1, fp);
    fread(&cart->dest_code, 1, 1, fp);
    fread(&cart->old_licensee_code, 1, 1, fp);
    fread(&cart->mask_rom_version_numer, 1, 1, fp);
    fread(&cart->header_checksum, 1, 1, fp);
    fread(&cart->global_checksum, 2, 1, fp);

    return;
}

//determine the type of cartridge and set mapper functions
void select_mapper(uint8_t cart_type, mapper_t *mapper){
    switch(cart_type){
    case ROM_ONLY:
        //mapper->read_ROM = (*)lol();
        break;
    case MBC1:
        mapper->read = read_MBC1;
        mapper->write = write_MBC1;
        break;
    case MBC1_RAM:
        break;
    case MBC1_RAM_BATTERY:
        break;
    case MBC2:
        break;
    case MBC2_BATTERY:
        break;
    case ROM_RAM:
        break;
    case ROM_RAM_BATTERY:
        break;
    case MMM01:
        break;
    case MMM01_RAM:
        break;
    case MMM01_RAM_BATTERY:
        break;
    case MBC3_TIMER_BATTERY:
        break;
    case MBC3_TIMER_RAM_BATTERY:
        break;
    case MBC3:
        break;
    case MBC3_RAM:
        break;
    case MBC3_RAM_BATTERY:
        break;
    case MBC5:
        break;
    case MBC5_RAM:
        break;
    case MBC5_RAM_BATTERY:
        break;
    case MBC5_RUMBLE:
        break;
    case MBC5_RUMBLE_RAM:
        break;
    case MBC5_RUMBLE_RAM_BATTERY:
        break;
    case MBC6:
        break;
    case MBC7_SENSOR_RUMBLE_RAM_BATTERY:
        break;
    case POCKET_CAMERA:
        break;
    case BANDAI_TAMA5:
        break;
    case HuC3:
        break;
    case HuC1_RAM_BATTERY:
        break;
    default:
        LOG(ERROR, "Cartridge not recognized");
        exit(1);
    }
}

