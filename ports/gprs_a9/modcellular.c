#include "modcellular.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_info.h"
#include "api_sim.h"
#include "api_sms.h"
#include "api_os.h"
#include "buffer.h"

#define NTW_REG_BIT 0x01
#define NTW_ROAM_BIT 0x02

// ------
// Notify
// ------

// Tracks SIM status on the network
int sim_status = 0;

// A buffer used for listing messages
mp_obj_list_t* sms_list_buffer = NULL;
uint8_t sms_list_buffer_count = 0;

void notify_no_sim(API_Event_t* event) {
    sim_status = 0;
}

void notify_registered_home(API_Event_t* event) {
    sim_status = NTW_REG_BIT;
}

void notify_registered_roaming(API_Event_t* event) {
    sim_status = NTW_REG_BIT | NTW_ROAM_BIT;
}

STATIC mp_obj_t sms_from_record(SMS_Message_Info_t* record);

void notify_sms_list(API_Event_t* event) {
    SMS_Message_Info_t* messageInfo = (SMS_Message_Info_t*)event->pParam1;

    if (sms_list_buffer && (sms_list_buffer->len > sms_list_buffer_count)) {
        sms_list_buffer->items[sms_list_buffer_count] = sms_from_record(messageInfo);
        sms_list_buffer_count ++;
    }
    OS_Free(messageInfo->data);
}

// -------
// Classes
// -------

typedef struct _sms_obj_t {
    mp_obj_base_t base;
    uint8_t index;
    uint8_t status;
    uint8_t phone_number_type;
    mp_obj_t phone_number;
    mp_obj_t message;
} sms_obj_t;

//mp_obj_t sms_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    //enum { ARG_phone_number, ARG_message };
    //static const mp_arg_t allowed_args[] = {
        //{ MP_QSTR_phone_number, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        //{ MP_QSTR_message, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    //};

    //mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    //mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    //sms_obj_t *self = m_new_obj(sms_obj_t);
    //self->base.type = type;
    
    //const char* phone_number = mp_obj_str_get_str(args[ARG_phone_number].u_obj);
    //uint16_t phone_number_len = strlen(phone_number);
    //if (phone_number_len > SMS_PHONE_NUMBER_MAX_LEN - 1) {
        //char msg[64];
        //snprintf(msg, sizeof(msg), "The length of the phone number %d exceeds the maximal value %d", phone_number_len, SMS_PHONE_NUMBER_MAX_LEN - 1);
        //mp_raise_ValueError(msg);
        //return mp_const_none;
    //}
    //memcpy(self->phone_number, phone_number, phone_number_len + 1);

    //const char* message = mp_obj_str_get_str(args[ARG_message].u_obj);
    //self->message_len = strlen(message) + 1;
    //self->message = OS_Malloc(self->message_len);
    //memcpy(self->message, message, self->message_len);
    
    //return MP_OBJ_FROM_PTR(self);
//}

uint8_t bitsum(uint32_t i) {
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

STATIC mp_obj_t sms___status__(mp_obj_t self_in) {
    // ========================================
    // SMS status.
    // Returns:
    //     Status as int.
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->status);
}

MP_DEFINE_CONST_FUN_OBJ_1(sms___status___obj, sms___status__);

STATIC mp_obj_t sms_inbox(mp_obj_t self_in) {
    // ========================================
    // Determines if SMS is inbox or outbox.
    // Returns:
    //     True if inbox.
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t s = self->status;
    REQUIRES_VALID_SMS_STATUS(s);
    return mp_obj_new_bool(s & (SMS_STATUS_READ | SMS_STATUS_UNREAD));
}

MP_DEFINE_CONST_FUN_OBJ_1(sms_inbox_obj, sms_inbox);

STATIC mp_obj_t sms_unread(mp_obj_t self_in) {
    // ========================================
    // Determines if SMS is unread.
    // Returns:
    //     True if unread.
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t s = self->status;
    REQUIRES_VALID_SMS_STATUS(s);
    return mp_obj_new_bool(s & SMS_STATUS_UNREAD);
}

MP_DEFINE_CONST_FUN_OBJ_1(sms_unread_obj, sms_unread);

STATIC mp_obj_t sms_sent(mp_obj_t self_in) {
    // ========================================
    // Determines if SMS was sent.
    // Returns:
    //     True if it was sent.
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t s = self->status;
    REQUIRES_VALID_SMS_STATUS(s);
    return mp_obj_new_bool(!(s | SMS_STATUS_UNSENT));
}

MP_DEFINE_CONST_FUN_OBJ_1(sms_sent_obj, sms_sent);

STATIC void sms_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // ========================================
    // SMS.__str__()
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_hal_stdout_tx_str("SMS(\"");
    mp_hal_stdout_tx_str(mp_obj_str_get_str(self->phone_number));
    mp_hal_stdout_tx_str("\"(");
    switch (self->phone_number_type) {
        case SMS_NUMBER_TYPE_UNKNOWN:
            mp_hal_stdout_tx_str("unk");
            break;
        case SMS_NUMBER_TYPE_INTERNATIONAL:
            mp_hal_stdout_tx_str("int");
            break;
        case SMS_NUMBER_TYPE_NATIONAL:
            mp_hal_stdout_tx_str("loc");
            break;
        default: {
            char msg[8];
            snprintf(msg, sizeof(msg), "%d", self->phone_number_type);
            mp_hal_stdout_tx_str(msg);
            break;
        }
    }
    mp_hal_stdout_tx_str("), \"");
    mp_hal_stdout_tx_str(mp_obj_str_get_str(self->message));
    mp_hal_stdout_tx_str("\", unread: ");
    if (sms_unread(self_in)) {
        mp_hal_stdout_tx_str("True");
    } else {
        mp_hal_stdout_tx_str("False");
    }
    mp_hal_stdout_tx_str(", sent: ");
    if (sms_sent(self_in)) {
        mp_hal_stdout_tx_str("True");
    } else {
        mp_hal_stdout_tx_str("False");
    }
    mp_hal_stdout_tx_str(")");
}

STATIC const mp_rom_map_elem_t sms_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___status__), MP_ROM_PTR(&sms___status___obj) },
    { MP_ROM_QSTR(MP_QSTR_is_inbox), MP_ROM_PTR(&sms_inbox_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_unread), MP_ROM_PTR(&sms_unread_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_sent), MP_ROM_PTR(&sms_sent_obj) },
};

STATIC MP_DEFINE_CONST_DICT(sms_locals_dict, sms_locals_dict_table);

const mp_obj_type_t sms_type = {
    { &mp_type_type },
    .name = MP_QSTR_SMS,
    // .make_new = sms_make_new,
    .print = sms_print,
    .locals_dict = (mp_obj_dict_t*)&sms_locals_dict,
};

// -------
// Private
// -------

STATIC mp_obj_t sms_from_record(SMS_Message_Info_t* record) {
    // ========================================
    // Prepares an SMS object from the record.
    // Returns:
    //     A new SMS object.
    // ========================================
    sms_obj_t *self = m_new_obj_with_finaliser(sms_obj_t);
    self->base.type = &sms_type;
    self->index = record->index;
    self->status = (uint8_t)record->status;
    self->phone_number_type = (uint8_t)record->phoneNumberType;
    // Is this an SDK bug?
    self->phone_number = mp_obj_new_str(record->phoneNumber + 1, SMS_PHONE_NUMBER_MAX_LEN - 1);
    self->message = mp_obj_new_str(record->data, record->dataLen);
    return MP_OBJ_FROM_PTR(self);
}

// -------
// Methods
// -------

STATIC mp_obj_t get_imei(void) {
    // ========================================
    // Retrieves IMEI number.
    // Returns:
    //     IMEI number as a string.
    // ========================================
    char imei[16];
    memset(imei,0,sizeof(imei));
    INFO_GetIMEI((uint8_t*)imei);
    return mp_obj_new_str(imei, strlen(imei));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_imei_obj, get_imei);

STATIC mp_obj_t is_sim_present(void) {
    // ========================================
    // Checks whether the SIM card is inserted and ICCID can be retrieved.
    // Returns:
    //     True if SIM present.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    return mp_obj_new_bool(SIM_GetICCID((uint8_t*)iccid));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(is_sim_present_obj, is_sim_present);

STATIC mp_obj_t is_network_registered(void) {
    // ========================================
    // Checks whether registered on the cellular network.
    // Returns:
    //     True if registered.
    // ========================================
    return mp_obj_new_bool(sim_status & NTW_REG_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(is_network_registered_obj, is_network_registered);

STATIC mp_obj_t is_roaming(void) {
    // ========================================
    // Checks whether registered on the roaming network.
    // Returns:
    //     True if roaming.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION;

    return mp_obj_new_bool(sim_status & NTW_ROAM_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(is_roaming_obj, is_roaming);

STATIC mp_obj_t get_iccid(void) {
    // ========================================
    // Retrieves ICCID number.
    // Returns:
    //     ICCID number as a string.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    if (SIM_GetICCID((uint8_t*)iccid))
        return mp_obj_new_str(iccid, strlen(iccid));
    else {
        mp_raise_ValueError("No ICCID data: is SIM card inserted?");
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_iccid_obj, get_iccid);

STATIC mp_obj_t get_imsi(void) {
    // ========================================
    // Retrieves IMSI number.
    // Returns:
    //     IMSI number as a string.
    // ========================================
    char imsi[21];
    memset(imsi, 0, sizeof(imsi));
    if (SIM_GetIMSI((uint8_t*)imsi))
        return mp_obj_new_str(imsi, strlen(imsi));
    else {
        mp_raise_ValueError("No IMSI data: is SIM card inserted?");
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_imsi_obj, get_imsi);

STATIC mp_obj_t sms_list(void) {
    // ========================================
    // Lists SMS messages.
    // Returns:
    //     A list of SMS messages.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION;

    SMS_Storage_Info_t storage;
    SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);
    
    sms_list_buffer = mp_obj_new_list(storage.used, NULL);
    sms_list_buffer_count = 0;

    SMS_ListMessageRequst(SMS_STATUS_ALL, SMS_STORAGE_SIM_CARD);
    
    uint8_t attempts = 0;
    for (attempts = 0; attempts < MAX_SMS_LIST_ATTEMPTS; attempts++) {
        OS_Sleep(100);
        if (sms_list_buffer_count == storage.used) break;
    }
    
    mp_obj_list_t *result = sms_list_buffer;
    sms_list_buffer = NULL;
    
    uint16_t i;
    for (i = sms_list_buffer_count; i < result->len; i++) {
        result->items[i] = mp_const_none;
    }
    sms_list_buffer_count = 0;

    return (mp_obj_t)result;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(sms_list_obj, sms_list);

STATIC mp_obj_t sms_send(mp_obj_t destination, mp_obj_t message) {
    // ========================================
    // Sends an SMS messsage.
    // Args:
    //     destination (str): telephone number;
    //     message (str): message contents;
    // ========================================
    REQUIRES_NETWORK_REGISTRATION;

    const char* destination_c = mp_obj_str_get_str(destination);
    const char* message_c = mp_obj_str_get_str(message);

    if (!SMS_SetFormat(SMS_FORMAT_TEXT, SIM0)) {
        mp_raise_ValueError("Failed to set SMS format: is SIM card present?");
        return mp_const_none;
    }

    SMS_Parameter_t smsParam = {
        .fo = 17 , // stadard values
        .vp = 167,
        .pid= 0  ,
        .dcs= 8  , // 0:English 7bit, 4:English 8 bit, 8:Unicode 2 Bytes
    };
    if (!SMS_SetParameter(&smsParam,SIM0)) {
        mp_raise_ValueError("Failed to set SMS parameters");
        return mp_const_none;
    }

    if (!SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD)) {
        mp_raise_ValueError("Failed to set SMS storage in the SIM card");
        return mp_const_none;
    }

    uint8_t* unicode = NULL;
    uint32_t unicodeLen;

    if (!SMS_LocalLanguage2Unicode((uint8_t*)message_c, strlen(message_c), CHARSET_UTF_8, &unicode, &unicodeLen)) {
        mp_raise_ValueError("Failed to convert to Unicode before sending SMS");
        return mp_const_none;
    }

    if (!SMS_SendMessage(destination_c, unicode, unicodeLen, SIM0)) {
        OS_Free(unicode);
        mp_raise_ValueError("Failed to send SMS message");
        return mp_const_none;
    }
    OS_Free(unicode);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(sms_send_obj, sms_send);

STATIC const mp_map_elem_t mp_module_cellular_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cellular) },

    { MP_ROM_QSTR(MP_QSTR_SMS), MP_ROM_PTR(&sms_type) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imei), (mp_obj_t)&get_imei_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_sim_present), (mp_obj_t)&is_sim_present_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_network_registered), (mp_obj_t)&is_network_registered_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_roaming), (mp_obj_t)&is_roaming_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_iccid), (mp_obj_t)&get_iccid_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imsi), (mp_obj_t)&get_imsi_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_list), (mp_obj_t)&sms_list_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_send), (mp_obj_t)&sms_send_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_cellular_globals, mp_module_cellular_globals_table);

const mp_obj_module_t cellular_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cellular_globals,
};
